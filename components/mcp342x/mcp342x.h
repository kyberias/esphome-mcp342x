#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace mcp342x {

class MCP342xComponent : public PollingComponent, public i2c::I2CDevice {
 public:
  void set_gain_code(uint8_t gain_code) { this->gain_code_ = (gain_code & 0x03); }
  void set_resolution_code(uint8_t res_code) { this->res_code_ = (res_code & 0x03); }
  void set_vref(float vref) { this->vref_ = vref; }

  void register_channel_sensor(uint8_t channel, sensor::Sensor *s);

  void setup() override;
  void update() override;

 protected:
  sensor::Sensor *channels_[4] = {nullptr, nullptr, nullptr, nullptr};

  // MCP342x config
  uint8_t gain_code_{0};  // 0..3 => 1,2,4,8
  uint8_t res_code_{2};   // 0..3 => 12,14,16,18
  float vref_{2.048f};

  // State machine for round-robin one-shot conversions
  uint8_t cur_ch_{0};
  bool conversion_started_{false};
  uint32_t started_ms_{0};

  void start_conversion_(uint8_t channel);
  bool read_conversion_(int32_t &raw, bool &ready, uint8_t &cfg_out);

  float lsb_volts_() const;
  float gain_value_() const;
  uint8_t config_byte_(uint8_t channel, bool start) const;
};

}
}
