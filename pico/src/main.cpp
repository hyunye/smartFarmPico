#include <Arduino.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <BH1750.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

#define ONE_WIRE_BUS 15
#define MOISTURE_PIN 26

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
BH1750 lightMeter;
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Server-controlled interval. The server sends the default (60 minutes) after BLE connects.
unsigned long measurementIntervalMs = 60UL * 60UL * 1000UL;
unsigned long lastMeasurementAt = 0;
bool measureRequested = true;
String commandBuffer;

void processCommand(const String& line) {
    StaticJsonDocument<160> command;
    if (deserializeJson(command, line)) return;
    const char* type = command["command"] | "";

    if (strcmp(type, "setMeasurementInterval") == 0 && command["minutes"].is<unsigned long>()) {
        unsigned long minutes = command["minutes"];
        if (minutes >= 1 && minutes <= 1440) measurementIntervalMs = minutes * 60UL * 1000UL;
    } else if (strcmp(type, "measureNow") == 0) {
        measureRequested = true;
    }
}

void receiveCommands() {
    while (Serial1.available()) {
        char value = (char)Serial1.read();
        if (value == '\n') { processCommand(commandBuffer); commandBuffer = ""; }
        else if (value != '\r' && commandBuffer.length() < 159) commandBuffer += value;
        else commandBuffer = "";
    }
}

void measureAndSend() {
    sensors.requestTemperatures();
    const float temperature = sensors.getTempCByIndex(0);
    const float lux = lightMeter.readLightLevel();
    const int rawMoisture = analogRead(MOISTURE_PIN);
    const float moisturePercent = constrain(map(rawMoisture, 4000, 1500, 0, 100), 0, 100);

    lcd.setCursor(0, 0); lcd.print("Temp:  "); lcd.print(temperature, 1); lcd.print(" C  ");
    lcd.setCursor(0, 1); lcd.print("Moist: "); lcd.print(moisturePercent, 0); lcd.print(" %  ");
    lcd.setCursor(0, 2); lcd.print("Light: "); lcd.print(lux, 1); lcd.print(" lx ");
    lcd.setCursor(0, 3); lcd.print("Interval: "); lcd.print(measurementIntervalMs / 60000UL); lcd.print(" min  ");

    StaticJsonDocument<128> reading;
    reading["temperature"] = temperature;
    reading["moisture"] = moisturePercent;
    reading["light"] = lux;
    serializeJson(reading, Serial1); Serial1.println();
    serializeJson(reading, Serial); Serial.println();
}

void setup() {
    Serial.begin(115200);
    Serial1.begin(9600);
    Wire.setSDA(4); Wire.setSCL(5); Wire.begin();
    sensors.begin();
    lightMeter.begin();
    lcd.init(); lcd.backlight();
    lcd.setCursor(0, 0); lcd.print("Pico Monitoring");
}

void loop() {
    receiveCommands();
    const unsigned long now = millis();
    if (measureRequested || now - lastMeasurementAt >= measurementIntervalMs) {
        measureRequested = false;
        lastMeasurementAt = now;
        measureAndSend();
    }
    delay(20);
}
