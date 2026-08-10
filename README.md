# SmartFarm Pico

PlatformIO firmware for the sensor node.

## Wiring

- DS18B20 temperature sensor: GP15
- Soil-moisture analog output: GP26
- BH1750/LCD I2C: SDA GP4, SCL GP5
- BLE UART module: UART1 GP0/GP1 at 9600 baud
- Pump relay: GP16, active HIGH by default

The Bluetooth module must be a BLE UART module with a notify characteristic for Pico TX and a writable characteristic for Pico RX. Classic Bluetooth-only HC-05/HC-06 modules are incompatible with the server.

The firmware sends a JSON reading every two seconds and accepts newline-delimited JSON commands. On startup, the relay is explicitly turned off. Change `PUMP_PIN` or `PUMP_ACTIVE_HIGH` in `pico/src/main.cpp` to match the relay board before uploading.
