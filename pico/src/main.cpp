#include <Arduino.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <BH1750.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

// 핀 설정
#define ONE_WIRE_BUS 15
#define MOISTURE_PIN 26

// 객체 초기화
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
BH1750 lightMeter;
LiquidCrystal_I2C lcd(0x27, 20, 4);

void setup() {
    // 1. 시리얼 초기화
    Serial.begin(115200);   // PC 디버깅용
    Serial1.begin(9600);    // 외부 블루투스 모듈용 (GP0, GP1 사용)

    // 2. I2C 초기화 (SDA: GP4, SCL: GP5)
    Wire.setSDA(4);
    Wire.setSCL(5);
    Wire.begin();

    // 3. 센서 시작
    sensors.begin();
    lightMeter.begin();
    
    // 4. LCD 초기화
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Pico Monitoring");
}

void loop() {
    // 온도 읽기
    sensors.requestTemperatures();
    float temperature = sensors.getTempCByIndex(0);

    // 조도 읽기
    float lux = lightMeter.readLightLevel();

    // 습도 읽기 (ADC 12비트 0-4095 기준)
    int rawMoisture = analogRead(MOISTURE_PIN);
    float moisturePercent = map(rawMoisture, 4000, 1500, 0, 100);
    if (moisturePercent > 100) moisturePercent = 100;
    if (moisturePercent < 0) moisturePercent = 0;

    // --- LCD 표시 ---
    lcd.setCursor(0, 0);
    lcd.print("Temp:  "); lcd.print(temperature, 1); lcd.print(" C  ");
    lcd.setCursor(0, 1);
    lcd.print("Moist: "); lcd.print(moisturePercent, 0); lcd.print(" %  ");
    lcd.setCursor(0, 2);
    lcd.print("Light: "); lcd.print(lux, 1); lcd.print(" lx ");

    // --- 블루투스 전송 (JSON 포맷) ---
    StaticJsonDocument<128> doc;
    doc["temperature"] = temperature;
    doc["moisture"] = moisturePercent;
    doc["light"] = lux;

    String jsonString;
    serializeJson(doc, jsonString);
    
    Serial1.println(jsonString); // GP0 핀을 통해 블루투스 모듈로 전송
    Serial.println(jsonString);   // USB 시리얼 모니터에 출력

    delay(2000);
}