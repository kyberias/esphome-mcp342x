# ESPHome MCP342x (MCP3424 etc.) external component

Provides per-channel voltage sensors for MCP342x I2C ADCs.
Designed for multi-channel time-sliced one-shot conversions (non-blocking).

Example:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/kyberias/esphome-mcp342x
      ref: main
    components: [ mcp342x ]

i2c:

mcp342x:
  - id: adc1
    address: 0x68
    gain: 1
    resolution: 16
    update_interval: 200ms

sensor:
  - platform: mcp342x
    mcp342x_id: adc1
    channel: 0
    name: "CH0 Voltage"
