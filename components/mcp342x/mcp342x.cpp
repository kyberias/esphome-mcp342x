#include "mcp342x.h"
#include "esphome/core/log.h"

#ifdef ARDUINO_ARCH_ESP8266
#include <Wire.h>
#endif

namespace esphome {
namespace mcp342x {

static const char *const TAG = "mcp342x";

void MCP342xComponent::register_channel_sensor(uint8_t channel, sensor::Sensor *s) {
  if (channel > 3) return;
  this->channels[channel] = s;
}

void MCP342xComponent::setup() {
#ifdef ARDUINO_ARCH_ESP8266
  // Default can be too low for some stretching devices; 150ms is a common safe value.
  Wire.setClockStretchLimit(150000);
#endif 

  // Nothing to do here; we will run one-shot conversions per channel in update().
  this->conversion_started = false;
}

void MCP342xComponent::update() {
  if (this->conversion_started) {
    if (millis() - this->started_ms < this->conversion_time_ms())
        return;  // don't read yet
  }

    // Find next enabled channel if current is not configured
  uint8_t tries = 0;
  while (tries < 4 && this->channels[this->cur_ch] == nullptr) {
    this->cur_ch = (this->cur_ch + 1) & 0x03;
    tries++;
  }
  if (tries >= 4) return;  // no sensors registered

  if (!this->conversion_started) {
    this->start_conversion(this->cur_ch);
    this->conversion_started = true;
    this->started_ms = millis();
    return;
  }

  int32_t raw = 0;
  bool ready = false;
  uint8_t cfg = 0;
  if (!this->read_conversion(raw, ready, cfg)) {
    ESP_LOGW(TAG, "I2C read failed");
    this->conversion_started = false;
    return;
  }
  if (!ready) {
    // Not ready yet; try again next update tick.
    // (At 18-bit, this will often take multiple ticks.)
    return;
  }

  // Convert raw code -> volts (signed). Most single-ended divider use is positive.
  const float v = raw * this->lsb_volts();
  if (this->channels[this->cur_ch] != nullptr) {
    this->channels[this->cur_ch]->publish_state(v);
  }

  // Move to next channel
  this->cur_ch = (this->cur_ch + 1) & 0x03;
  this->conversion_started = false;
}

void MCP342xComponent::start_conversion(uint8_t channel) {
  uint8_t cfg = this->config_byte(channel, true);
  if (this->write(&cfg, 1) != i2c::NO_ERROR) {
    ESP_LOGW(TAG, "I2C write (start conversion) failed");
  }
}

bool MCP342xComponent::read_conversion(int32_t &raw, bool &ready, uint8_t &cfg_out) {
  // 18-bit: 3 data bytes + config; else 2 data + config
  const bool is18 = (this->res_code == 3);
  const uint8_t len = is18 ? 4 : 3;
  uint8_t buf[4] = {0};

  if (this->read(buf, len) != i2c::NO_ERROR) {
    return false;
  }

  cfg_out = buf[len - 1];
  // RDY bit (bit7): 0 = ready, 1 = not ready (in one-shot conversion)
  ready = ((cfg_out & 0x80) == 0);

  if (is18) {
    // 3 bytes signed, top-aligned; shift down 6 to get 18-bit signed code
    int32_t v = ((int32_t) buf[0] << 16) | ((int32_t) buf[1] << 8) | (int32_t) buf[2];
    if (v & 0x800000) v |= 0xFF000000;  // sign extend 24->32
    raw = v >> 6;
  } else {
    int16_t v = ((int16_t) buf[0] << 8) | buf[1];
    int32_t vv = (int32_t) v;

    // shift down for 12/14-bit (data left-justified in 16-bit)
    if (this->res_code == 0) vv >>= 4;      // 12-bit
    else if (this->res_code == 1) vv >>= 2; // 14-bit
    // 16-bit: no shift

    raw = vv;
  }

  return true;
}

float MCP342xComponent::gain_value() const {
  switch (this->gain_code) {
    case 0: return 1.0f;
    case 1: return 2.0f;
    case 2: return 4.0f;
    case 3: return 8.0f;
    default: return 1.0f;
  }
}

float MCP342xComponent::lsb_volts() const {
  // LSB = (2*Vref/gain) / 2^(N-1)
  int n = 16;
  if (this->res_code == 0) n = 12;
  else if (this->res_code == 1) n = 14;
  else if (this->res_code == 2) n = 16;
  else if (this->res_code == 3) n = 18;

  const float fs = (2.0f * this->vref) / this->gain_value();
  const float denom = (float) (1UL << (n - 1));  // n<=18 safe in 32-bit
  return fs / denom;
}

uint8_t MCP342xComponent::config_byte(uint8_t channel, bool start) const {
  // bit7 RDY: in one-shot mode, writing 1 starts conversion
  // bits6-5 channel
  // bit4 mode: 0 = one-shot
  // bits3-2 sample rate/resolution
  // bits1-0 gain
  uint8_t cfg = 0;
  if (start) cfg |= 0x80;
  cfg |= (channel & 0x03) << 5;
  // one-shot => bit4 = 0
  cfg |= (this->res_code & 0x03) << 2;
  cfg |= (this->gain_code & 0x03);
  return cfg;
}

uint32_t MCP342xComponent::conversion_time_ms() const {
  switch (this->res_code) {
    case 0: return 5;    // 12-bit ~240 SPS
    case 1: return 20;   // 14-bit ~60 SPS
    case 2: return 80;   // 16-bit ~15 SPS (give it margin)
    case 3: return 320;  // 18-bit ~3.75 SPS (give it margin)
    default: return 80;
  }
}

}  // namespace mcp342x
}  // namespace esphome
