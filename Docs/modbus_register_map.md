# Modbus Register Map

Sensor definitions are loaded at runtime from `config.json` on the SD card.

## config.json sensor entry format

```json
{
  "sensors": [
    {
      "name":      "temperature",
      "enabled":   1,
      "slave":     1,
      "function":  4,
      "register":  0,
      "scale":     0.1,
      "offset":    0.0
    }
  ]
}
```

| Field      | Description                                   |
|------------|-----------------------------------------------|
| name       | Label used in CSV header and debug output      |
| enabled    | 1 = poll this sensor, 0 = skip                |
| slave      | Modbus slave address (1-247)                   |
| function   | 3 = holding registers, 4 = input registers     |
| register   | Starting register address (0-based)            |
| scale      | Raw value multiplier                           |
| offset     | Added after scaling: `value = raw * scale + offset` |
