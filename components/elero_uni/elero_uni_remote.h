#pragma once

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

#include "esphome/core/log.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_attr.h"
#include "esp_err.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "nvs.h"

#include "elero_uni_protocol.h"

// ESPHome / ESP-IDF reference transmitter for the reverse-engineered
// unidirectional Elero UNI 868 MHz protocol.
//
// RF timing, CC1101 registers and rolling-index handling are extracted from the
// implementation that was validated on real Elero UNI receivers.
//
// IMPORTANT: configure a unique sender identity before pairing. Never reset or
// reuse the rolling index of an already paired virtual sender.

namespace elero_uni_remote {

static const char *const TAG = "elero.uni.tx";

// Tested ESP32 DevKit wiring.
static constexpr int PIN_SCK = 18;
static constexpr int PIN_MISO = 19;
static constexpr int PIN_MOSI = 23;
static constexpr int PIN_CS = 17;
static constexpr int PIN_GDO0 = 16;  // synchronous serial DATA
static constexpr int PIN_GDO2 = 27;  // synchronous serial CLOCK

static constexpr const char *NVS_NAMESPACE = "elero_uni_v1";
static constexpr uint8_t MAX_REMOTES = 8;

struct RegValue {
  uint8_t addr;
  uint8_t value;
};

struct RemoteConfig {
  bool configured{false};
  char name[32]{};
  char nvs_key[16]{};
  uint16_t initial_index{1};
  uint8_t type{0x20};
  uint8_t id0{0};
  uint8_t id1{0};
  uint8_t id2{0};
};

struct TxGroup {
  std::array<uint8_t, 128> bytes{};
  uint16_t data_bits{0};
  uint16_t total_bits{0};
  bool ok{false};
};

static std::array<RemoteConfig, MAX_REMOTES> remotes{};
static std::array<std::string, MAX_REMOTES> last_tx{};
static spi_device_handle_t spi_dev = nullptr;
static SemaphoreHandle_t radio_mutex = nullptr;
static bool radio_ready = false;
static bool gpio_isr_ready = false;

static volatile bool tx_active = false;
static volatile bool tx_done = false;
static volatile uint16_t tx_index = 0;
static volatile uint16_t tx_count = 0;
static const uint8_t *volatile tx_bytes = nullptr;

static TxGroup press_group{};
static TxGroup release_group{};

inline uint64_t micros64_() {
  return static_cast<uint64_t>(esp_timer_get_time());
}

class RadioGuard {
 public:
  explicit RadioGuard(uint32_t timeout_ms = 3000) {
    if (radio_mutex != nullptr) {
      locked_ = xSemaphoreTake(radio_mutex, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
    }
  }
  ~RadioGuard() {
    if (locked_ && radio_mutex != nullptr) xSemaphoreGive(radio_mutex);
  }
  explicit operator bool() const { return locked_; }

 private:
  bool locked_{false};
};

inline bool spi_select_() {
  if (spi_dev == nullptr) return false;
  gpio_set_level(static_cast<gpio_num_t>(PIN_CS), 0);

  const uint64_t start = micros64_();
  while (gpio_get_level(static_cast<gpio_num_t>(PIN_MISO)) != 0) {
    if ((micros64_() - start) > 100000ULL) {
      gpio_set_level(static_cast<gpio_num_t>(PIN_CS), 1);
      ESP_LOGE(TAG, "CC1101 MISO-ready timeout");
      return false;
    }
  }
  return true;
}

inline void spi_deselect_() {
  gpio_set_level(static_cast<gpio_num_t>(PIN_CS), 1);
}

inline bool spi_transfer_(const uint8_t *tx, uint8_t *rx, size_t len) {
  if (spi_dev == nullptr || tx == nullptr || len == 0) return false;
  spi_transaction_t t{};
  t.length = len * 8U;
  t.tx_buffer = tx;
  t.rx_buffer = rx;
  const esp_err_t err = spi_device_polling_transmit(spi_dev, &t);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "SPI transfer: %s", esp_err_to_name(err));
    return false;
  }
  return true;
}

inline uint8_t cc_strobe_(uint8_t command) {
  if (!spi_select_()) return 0xFF;
  uint8_t rx = 0xFF;
  const bool ok = spi_transfer_(&command, &rx, 1);
  spi_deselect_();
  return ok ? rx : 0xFF;
}

inline uint8_t cc_read_reg_(uint8_t addr) {
  if (!spi_select_()) return 0xFF;
  uint8_t tx[2] = {static_cast<uint8_t>(addr | 0x80U), 0};
  uint8_t rx[2] = {0xFF, 0xFF};
  const bool ok = spi_transfer_(tx, rx, 2);
  spi_deselect_();
  return ok ? rx[1] : 0xFF;
}

inline uint8_t cc_read_status_(uint8_t addr) {
  if (!spi_select_()) return 0xFF;
  uint8_t tx[2] = {static_cast<uint8_t>(addr | 0xC0U), 0};
  uint8_t rx[2] = {0xFF, 0xFF};
  const bool ok = spi_transfer_(tx, rx, 2);
  spi_deselect_();
  return ok ? rx[1] : 0xFF;
}

inline bool cc_write_reg_(uint8_t addr, uint8_t value) {
  if (!spi_select_()) return false;
  uint8_t tx[2] = {addr, value};
  const bool ok = spi_transfer_(tx, nullptr, 2);
  spi_deselect_();
  return ok;
}

inline bool cc_write_burst_(uint8_t addr, const uint8_t *data, size_t len) {
  if (data == nullptr || len == 0 || len > 8) return false;
  if (!spi_select_()) return false;
  uint8_t tx[9]{};
  tx[0] = static_cast<uint8_t>(addr | 0x40U);
  for (size_t i = 0; i < len; i++) tx[i + 1] = data[i];
  const bool ok = spi_transfer_(tx, nullptr, len + 1);
  spi_deselect_();
  return ok;
}

// Register set from the validated 868.300 MHz synchronous serial sender.
static constexpr RegValue REGS[] = {
    {0x0B,0x06},{0x0C,0x00},
    {0x0D,0x21},{0x0E,0x65},{0x0F,0x6A},
    {0x10,0x56},{0x11,0x83},{0x12,0x00},
    {0x13,0x52},{0x14,0xF8},{0x0A,0x00},{0x15,0x43},
    {0x21,0x56},{0x22,0x10},{0x18,0x18},{0x17,0x00},
    {0x19,0x1F},{0x1A,0x6C},
    {0x1B,0x4B},{0x1C,0x40},{0x1D,0x91},
    {0x23,0xE9},{0x24,0x2A},{0x25,0x00},{0x26,0x1F},
    {0x29,0x59},{0x2C,0x81},{0x2D,0x35},{0x2E,0x09},
    {0x00,0x0B},{0x02,0x0C},
    {0x07,0x0C},{0x08,0x12},{0x09,0x00},{0x06,0xFF},
    {0x04,0xD3},{0x05,0x91},
};

inline bool configure_radio_() {
  cc_strobe_(0x36);  // SIDLE
  esp_rom_delay_us(300);

  bool ok = true;
  for (const auto &r : REGS) ok &= cc_write_reg_(r.addr, r.value);

  const uint8_t pa = 0xC2;
  ok &= cc_write_burst_(0x3E, &pa, 1);  // PATABLE

  cc_strobe_(0x33);  // SCAL
  vTaskDelay(pdMS_TO_TICKS(5));
  cc_strobe_(0x36);  // SIDLE

  for (const auto &r : REGS) {
    if (r.addr >= 0x23 && r.addr <= 0x26) continue;  // FSCAL changes after SCAL
    if (cc_read_reg_(r.addr) != r.value) ok = false;
  }
  return ok;
}

inline bool append_bit_(TxGroup &g, bool bit) {
  if (g.total_bits >= g.bytes.size() * 8U) return false;
  if (bit) {
    g.bytes[g.total_bits >> 3] |=
        static_cast<uint8_t>(1U << (7U - (g.total_bits & 7U)));
  }
  g.total_bits++;
  return true;
}

inline bool append_biphase_(TxGroup &g, bool logical) {
  // Validated mapping: logical 0 -> 01, logical 1 -> 10.
  return append_bit_(g, logical ? 1 : 0) &&
         append_bit_(g, logical ? 0 : 1);
}

inline bool build_group_(TxGroup &g, uint64_t code64, uint8_t repeats) {
  g.bytes.fill(0);
  g.data_bits = 0;
  g.total_bits = 0;
  g.ok = false;

  // Quiet lead-in used by the validated sender.
  for (uint8_t i = 0; i < 32; i++) if (!append_bit_(g, 0)) return false;

  for (uint8_t r = 0; r < repeats; r++) {
    const uint8_t prefix[8] = {0,0,0,0,1,1,1,1};
    for (uint8_t b : prefix) if (!append_bit_(g, b)) return false;

    // First decoded logical bit is fixed to zero.
    if (!append_biphase_(g, false)) return false;

    for (int bit = 63; bit >= 0; --bit) {
      if (!append_biphase_(g, ((code64 >> bit) & 1ULL) != 0)) return false;
    }
  }

  for (uint8_t i = 0; i < 32; i++) if (!append_bit_(g, 0)) return false;
  g.data_bits = g.total_bits;

  // The working transmitter appended 16 zero/invalid flush clocks.
  for (uint8_t i = 0; i < 16; i++) if (!append_bit_(g, 0)) return false;

  g.ok = (g.data_bits == static_cast<uint16_t>(64U + 138U * repeats)) &&
         (g.total_bits == static_cast<uint16_t>(g.data_bits + 16U));
  return g.ok;
}

static void IRAM_ATTR clock_isr_(void *arg) {
  (void) arg;
  if (!tx_active || tx_bytes == nullptr) return;

  const uint16_t next = static_cast<uint16_t>(tx_index + 1U);
  if (next < tx_count) {
    tx_index = next;
    const uint8_t bit =
        (tx_bytes[next >> 3] >> (7U - (next & 7U))) & 1U;
    gpio_set_level(static_cast<gpio_num_t>(PIN_GDO0), bit);
    return;
  }

  tx_active = false;
  tx_done = true;
}

inline bool init_gpio_isr_() {
  gpio_reset_pin(static_cast<gpio_num_t>(PIN_GDO0));
  gpio_set_direction(static_cast<gpio_num_t>(PIN_GDO0), GPIO_MODE_INPUT);

  gpio_reset_pin(static_cast<gpio_num_t>(PIN_GDO2));
  gpio_set_direction(static_cast<gpio_num_t>(PIN_GDO2), GPIO_MODE_INPUT);
  gpio_set_intr_type(static_cast<gpio_num_t>(PIN_GDO2), GPIO_INTR_NEGEDGE);

  esp_err_t err = gpio_install_isr_service(0);
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    ESP_LOGE(TAG, "gpio_install_isr_service: %s", esp_err_to_name(err));
    return false;
  }

  err = gpio_isr_handler_add(
      static_cast<gpio_num_t>(PIN_GDO2), clock_isr_, nullptr);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "gpio_isr_handler_add: %s", esp_err_to_name(err));
    return false;
  }

  gpio_intr_disable(static_cast<gpio_num_t>(PIN_GDO2));
  gpio_isr_ready = true;
  return true;
}

inline bool transmit_group_(const TxGroup &g) {
  if (!gpio_isr_ready || !g.ok || g.total_bits == 0) return false;

  cc_strobe_(0x36);  // SIDLE
  esp_rom_delay_us(300);
  gpio_set_direction(static_cast<gpio_num_t>(PIN_GDO0), GPIO_MODE_OUTPUT);

  tx_bytes = g.bytes.data();
  tx_index = 0;
  tx_count = g.total_bits;
  tx_done = false;
  tx_active = true;

  const uint8_t first = (g.bytes[0] >> 7) & 1U;
  gpio_set_level(static_cast<gpio_num_t>(PIN_GDO0), first);

  gpio_set_intr_type(static_cast<gpio_num_t>(PIN_GDO2), GPIO_INTR_NEGEDGE);
  gpio_intr_enable(static_cast<gpio_num_t>(PIN_GDO2));
  const uint64_t start = micros64_();
  cc_strobe_(0x35);  // STX

  while (tx_active && (micros64_() - start) < 1000000ULL) {
    vTaskDelay(pdMS_TO_TICKS(1));
  }

  gpio_intr_disable(static_cast<gpio_num_t>(PIN_GDO2));
  cc_strobe_(0x36);  // SIDLE
  esp_rom_delay_us(300);
  gpio_set_direction(static_cast<gpio_num_t>(PIN_GDO0), GPIO_MODE_INPUT);

  const bool ok = tx_done && !tx_active;
  tx_bytes = nullptr;
  tx_active = false;
  return ok;
}

inline bool ensure_index_(uint8_t slot) {
  if (slot >= MAX_REMOTES || !remotes[slot].configured) return false;
  const auto &r = remotes[slot];

  nvs_handle_t h{};
  esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h);
  if (err != ESP_OK) return false;

  uint16_t value = 0;
  err = nvs_get_u16(h, r.nvs_key, &value);
  if (err == ESP_ERR_NVS_NOT_FOUND) {
    err = nvs_set_u16(h, r.nvs_key, r.initial_index);
    if (err == ESP_OK) err = nvs_commit(h);
    if (err == ESP_OK) {
      ESP_LOGW(TAG, "%s: NEXT initialized once to %u (0x%04X)",
               r.name, r.initial_index, r.initial_index);
    }
  }
  nvs_close(h);
  return err == ESP_OK;
}

inline bool peek_index(uint8_t slot, uint16_t *value) {
  if (slot >= MAX_REMOTES || !remotes[slot].configured || value == nullptr)
    return false;

  nvs_handle_t h{};
  if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &h) != ESP_OK) return false;
  const esp_err_t err = nvs_get_u16(h, remotes[slot].nvs_key, value);
  nvs_close(h);
  return err == ESP_OK;
}

inline bool reserve_pair_(uint8_t slot, uint16_t expected,
                          uint16_t *press, uint16_t *release) {
  if (slot >= MAX_REMOTES || !remotes[slot].configured ||
      press == nullptr || release == nullptr) return false;

  nvs_handle_t h{};
  esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h);
  if (err != ESP_OK) return false;

  uint16_t current = 0;
  err = nvs_get_u16(h, remotes[slot].nvs_key, &current);
  if (err == ESP_OK && current != expected) {
    ESP_LOGE(TAG, "%s: rolling-index race: expected %u, NVS %u; aborting",
             remotes[slot].name, expected, current);
    nvs_close(h);
    return false;
  }

  if (err == ESP_OK) {
    err = nvs_set_u16(h, remotes[slot].nvs_key,
                      static_cast<uint16_t>(current + 2U));
  }
  if (err == ESP_OK) err = nvs_commit(h);
  nvs_close(h);

  if (err != ESP_OK) return false;
  *press = current;
  *release = static_cast<uint16_t>(current + 1U);
  return true;
}

inline bool configure_remote(uint8_t slot, const char *name,
                             const char *nvs_key, uint16_t initial_index,
                             uint8_t type, uint8_t id0,
                             uint8_t id1, uint8_t id2) {
  if (slot >= MAX_REMOTES || name == nullptr || nvs_key == nullptr ||
      std::strlen(nvs_key) == 0 || std::strlen(nvs_key) >= 16) {
    return false;
  }

  auto &r = remotes[slot];
  std::memset(&r, 0, sizeof(r));
  std::snprintf(r.name, sizeof(r.name), "%s", name);
  std::snprintf(r.nvs_key, sizeof(r.nvs_key), "%s", nvs_key);
  r.initial_index = initial_index;
  r.type = type;
  r.id0 = id0;
  r.id1 = id1;
  r.id2 = id2;
  r.configured = true;
  last_tx[slot] = "No transmission yet";

  return ensure_index_(slot);
}

inline bool setup() {
  if (radio_ready) return true;

  gpio_reset_pin(static_cast<gpio_num_t>(PIN_CS));
  gpio_set_direction(static_cast<gpio_num_t>(PIN_CS), GPIO_MODE_OUTPUT);
  gpio_set_level(static_cast<gpio_num_t>(PIN_CS), 1);

  gpio_reset_pin(static_cast<gpio_num_t>(PIN_MISO));
  gpio_set_direction(static_cast<gpio_num_t>(PIN_MISO), GPIO_MODE_INPUT);

  spi_bus_config_t buscfg{};
  buscfg.mosi_io_num = PIN_MOSI;
  buscfg.miso_io_num = PIN_MISO;
  buscfg.sclk_io_num = PIN_SCK;
  buscfg.quadwp_io_num = -1;
  buscfg.quadhd_io_num = -1;
  buscfg.max_transfer_sz = 16;

  esp_err_t err = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_DISABLED);
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    ESP_LOGE(TAG, "spi_bus_initialize: %s", esp_err_to_name(err));
    return false;
  }

  spi_device_interface_config_t devcfg{};
  devcfg.clock_speed_hz = 4000000;
  devcfg.mode = 0;
  devcfg.spics_io_num = -1;
  devcfg.queue_size = 1;

  err = spi_bus_add_device(SPI2_HOST, &devcfg, &spi_dev);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "spi_bus_add_device: %s", esp_err_to_name(err));
    return false;
  }

  radio_mutex = xSemaphoreCreateMutex();
  if (radio_mutex == nullptr) {
    ESP_LOGE(TAG, "failed to create radio mutex");
    return false;
  }

  const uint8_t partnum = cc_read_status_(0x30);
  const uint8_t version = cc_read_status_(0x31);
  const bool chip_ok = partnum == 0x00 && version != 0x00 && version != 0xFF;

  const bool codec_ok =
      elero_uni::encode_code64(1, elero_uni::CMD_UP, false,
                               0x20, 0x70, 0x55, 0xC0).code64 ==
          0x54F4EE3C5DAE8902ULL &&
      elero_uni::encode_code64(2, elero_uni::CMD_UP, true,
                               0x20, 0x70, 0x55, 0xC0).code64 ==
          0x83AB3F5001D7CC1DULL;

  radio_ready = chip_ok && codec_ok && init_gpio_isr_() && configure_radio_();

  ESP_LOGW(TAG, "Elero UNI TX: %s | CC1101 PARTNUM=0x%02X VERSION=0x%02X",
           radio_ready ? "READY" : "ERROR", partnum, version);
  return radio_ready;
}

inline bool is_ready() { return radio_ready; }

inline bool send(uint8_t slot, uint8_t command, const char *command_name,
                 uint8_t press_repeats = 3, uint8_t release_repeats = 3) {
  if (!radio_ready || slot >= MAX_REMOTES || !remotes[slot].configured)
    return false;

  RadioGuard guard(3000);
  if (!guard) {
    last_tx[slot] = "ABORT - radio busy";
    return false;
  }

  uint16_t before = 0;
  if (!peek_index(slot, &before)) {
    last_tx[slot] = "ABORT - rolling index unavailable";
    return false;
  }

  const auto &r = remotes[slot];
  const auto press = elero_uni::encode_code64(
      before, command, false, r.type, r.id0, r.id1, r.id2);
  const auto release = elero_uni::encode_code64(
      static_cast<uint16_t>(before + 1U), command, true,
      r.type, r.id0, r.id1, r.id2);

  if (!build_group_(press_group, press.code64, press_repeats) ||
      !build_group_(release_group, release.code64, release_repeats)) {
    last_tx[slot] = "ABORT - frame build failed";
    return false;
  }

  if (!configure_radio_()) {
    last_tx[slot] = "ABORT - CC1101 configuration failed";
    return false;
  }

  // Critical safety rule: reserve and commit both indexes BEFORE the first RF
  // bit. A power loss may skip indexes, but cannot cause index reuse.
  uint16_t reserved_press = 0;
  uint16_t reserved_release = 0;
  if (!reserve_pair_(slot, before, &reserved_press, &reserved_release)) {
    last_tx[slot] = "ABORT - rolling-index reservation failed";
    return false;
  }

  ESP_LOGW(TAG,
      "%s -> %s | PRESS=%u RELEASE=%u | NEXT committed=%u",
      r.name, command_name, reserved_press, reserved_release,
      static_cast<uint16_t>(reserved_release + 1U));

  const bool press_ok = transmit_group_(press_group);
  vTaskDelay(pdMS_TO_TICKS(10));
  const bool release_ok = press_ok && transmit_group_(release_group);

  uint16_t next = 0;
  peek_index(slot, &next);

  char status[180];
  std::snprintf(status, sizeof(status), "%s | %s | %s | %u/%u -> NEXT %u",
                (press_ok && release_ok) ? "OK" : "TX ERROR",
                r.name, command_name, reserved_press, reserved_release, next);
  last_tx[slot] = status;
  ESP_LOGW(TAG, "%s", last_tx[slot].c_str());
  return press_ok && release_ok;
}

inline bool send_up(uint8_t slot) {
  return send(slot, elero_uni::CMD_UP, "UP", 3, 3);
}
inline bool send_stop(uint8_t slot) {
  return send(slot, elero_uni::CMD_STOP, "STOP", 3, 3);
}
inline bool send_down(uint8_t slot) {
  return send(slot, elero_uni::CMD_DOWN, "DOWN", 3, 3);
}
inline bool send_pairing_p(uint8_t slot) {
  return send(slot, elero_uni::CMD_P, "P", 8, 3);
}

inline std::string next_index_text(uint8_t slot) {
  uint16_t value = 0;
  if (!peek_index(slot, &value)) return "unavailable";
  char text[32];
  std::snprintf(text, sizeof(text), "%u (0x%04X)", value, value);
  return text;
}

inline std::string last_tx_text(uint8_t slot) {
  if (slot >= MAX_REMOTES || !remotes[slot].configured) return "unconfigured";
  return last_tx[slot];
}

inline void print_status(uint8_t slot) {
  if (slot >= MAX_REMOTES || !remotes[slot].configured) {
    ESP_LOGW(TAG, "remote slot %u is not configured", slot);
    return;
  }
  const auto &r = remotes[slot];
  ESP_LOGW(TAG, "%s | TYPE=0x%02X ID=%02X:%02X:%02X | NEXT=%s",
           r.name, r.type, r.id0, r.id1, r.id2,
           next_index_text(slot).c_str());
}

}  // namespace elero_uni_remote
