# SmartFarm Pico

PlatformIO firmware for the sensor node.

- DS18B20: GP15
- Soil-moisture analog output: GP26
- BH1750/LCD I2C: SDA GP4, SCL GP5
- BLE UART: UART1 GP0/GP1 at 9600 baud

The Pico starts with a 60-minute measurement interval. The server changes it using `{"command":"setMeasurementInterval","minutes":60}`. It sends one reading when connected (`measureNow`) and then measures only at the configured interval. There is no watering or pump-control function in this firmware.
