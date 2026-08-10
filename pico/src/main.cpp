#include <Arduino.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <BH1750.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

// Wiring: DS18B20 GP15, soil sensor GP26, I2C SDA GP4/SCL GP5,
// BLE UART RX/TX GP0/GP1, relay control GP16 (active HIGH by default).
#define ONE_WIRE_BUS 15
#define MOISTURE_PIN 26
#define PUMP_PIN 16
#define PUMP_ACTIVE_HIGH true

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
BH1750 lightMeter;
LiquidCrystal_I2C lcd(0x27, 20, 4);

bool watering = false;
unsigned long wateringStopsAt = 0;
String commandBuffer;

void setPump(bool enabled) {
    watering = enabled;
    digitalWrite(PUMP_PIN, (enabled == PUMP_ACTIVE_HIGH) ? HIGH : LOW);
}

// The BLE module must expose its UART RX as a writable BLE characteristic.
// Server commands are newline-delimited JSON, e.g. {"command":"water","enabled":true,"durationSeconds":10}.
void processCommand(const String& line) {
    StaticJsonDocument<192> command;
    if (deserializeJson(command, line)) return;

    const char* type = command["command"] | "";
    if (strcmp(type, "water") == 0 && command["enabled"].is<bool>()) {
        bool enabled = command["enabled"];
        setPump(enabled);
        int seconds = command["durationSeconds"] | 0;
        wateringStopsAt = enabled && seconds > 0 ? millis() + (unsigned long)seconds * 1000UL : 0;
    }
}

void receiveCommands() {
    while (Serial1.available()) {
        char value = (char)Serial1.read();
        if (value == '\n') {
            processCommand(commandBuffer);
            commandBuffer = "";
        } else if (value != '\r' && commandBuffer.length() < 191) {
            commandBuffer += value;
        } else {
            commandBuffer = "";
        }
    }
}

void setup() {
    Serial.begin(115200);
    Serial1.begin(9600);

    pinMode(PUMP_PIN, OUTPUT);
    setPump(false); // Fail safe: never enable the pump during boot.

    Wire.setSDA(4);
    Wire.setSCL(5);
    Wire.begin();
    sensors.begin();
    lightMeter.begin();

    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Pico Monitoring");
}

void loop() {
    receiveCommands();
    if (wateringStopsAt != 0 && (long)(millis() - wateringStopsAt) >= 0) {
        setPump(false);
        wateringStopsAt = 0;
    }

    sensors.requestTemperatures();
    float temperature = sensors.getTempCByIndex(0);
    float lux = lightMeter.readLightLevel();

    int rawMoisture = analogRead(MOISTURE_PIN);
    float moisturePercent = map(rawMoisture, 4000, 1500, 0, 100);
    moisturePercent = constrain(moisturePercent, 0, 100);

    lcd.setCursor(0, 0);
    lcd.print("Temp:  "); lcd.print(temperature, 1); lcd.print(" C  ");
    lcd.setCursor(0, 1);
    lcd.print("Moist: "); lcd.print(moisturePercent, 0); lcd.print(" %  ");
    lcd.setCursor(0, 2);
    lcd.print("Light: "); lcd.print(lux, 1); lcd.print(" lx ");
    lcd.setCursor(0, 3);
    lcd.print(watering ? "Pump: ON  " : "Pump: OFF ");

    StaticJsonDocument<160> reading;
    reading["temperature"] = temperature;
    reading["moisture"] = moisturePercent;
    reading["light"] = lux;
    reading["watering"] = watering;
    serializeJson(reading, Serial1);
    Serial1.println();
    serializeJson(reading, Serial);
    Serial.println();

    // Keep processing BLE commands during the measurement interval.
    const unsigned long waitUntil = millis() + 2000;
    while ((long)(millis() - waitUntil) < 0) {
        receiveCommands();
        if (wateringStopsAt != 0 && (long)(millis() - wateringStopsAt) >= 0) {
            setPump(false);
            wateringStopsAt = 0;
        }
        delay(10);
    }
}
