#include <Wire.h>
#include <U8g2lib.h>
#include <DHT.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

// -------------------- BLYNK & WIFI --------------------
char auth[] = "YOUR_BLYNK_AUTH_TOKEN";
char ssid[] = "YOUR_WIFI_SSID";
char pass[] = "YOUR_WIFI_PASSWORD";

// -------------------- OLED --------------------
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// -------------------- DHT22 --------------------
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// -------------------- SENSORS --------------------
#define SOIL_PIN 34
#define TRIG_PIN 12
#define ECHO_PIN 14
#define TANK_HEIGHT_CM 30

// -------------------- RELAYS --------------------
#define RELAY_MOTOR 26
#define RELAY_BULB 27
#define RELAY_MIST 25

// -------------------- BLYNK VIRTUAL PINS --------------------
#define VPIN_TEMP V0
#define VPIN_HUM  V1
#define VPIN_SOIL V2
#define VPIN_WATER V3
#define VPIN_MODE V4
#define VPIN_BULB V5
#define VPIN_MIST V6
#define VPIN_MOTOR V7

bool autoMode = true;

// -------------------- SETUP --------------------
void setup() {
  Serial.begin(115200);

  dht.begin();
  u8g2.begin();
  Blynk.begin(auth, ssid, pass);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(RELAY_MOTOR, OUTPUT);
  pinMode(RELAY_BULB, OUTPUT);
  pinMode(RELAY_MIST, OUTPUT);

  digitalWrite(RELAY_MOTOR, LOW);
  digitalWrite(RELAY_BULB, LOW);
  digitalWrite(RELAY_MIST, LOW);
}

// -------------------- BLYNK CONTROLS --------------------
BLYNK_WRITE(VPIN_MODE) {
  autoMode = param.asInt();
}

BLYNK_WRITE(VPIN_MOTOR) {
  if (!autoMode) digitalWrite(RELAY_MOTOR, param.asInt());
}

BLYNK_WRITE(VPIN_BULB) {
  if (!autoMode) digitalWrite(RELAY_BULB, param.asInt());
}

BLYNK_WRITE(VPIN_MIST) {
  if (!autoMode) digitalWrite(RELAY_MIST, param.asInt());
}

// -------------------- ULTRASONIC --------------------
long readDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return TANK_HEIGHT_CM;
  return duration * 0.034 / 2;
}

// -------------------- LOOP --------------------
void loop() {
  Blynk.run();

  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  int soilRaw = analogRead(SOIL_PIN);
  int soilPercent = map(soilRaw, 4095, 1500, 0, 100);
  soilPercent = constrain(soilPercent, 0, 100);

  long distance = readDistanceCM();
  int waterLevel = map(TANK_HEIGHT_CM - distance, 0, TANK_HEIGHT_CM, 0, 100);
  waterLevel = constrain(waterLevel, 0, 100);

  if (autoMode) {
    digitalWrite(RELAY_MOTOR, soilPercent < 30);
    digitalWrite(RELAY_BULB, temp < 20);
    digitalWrite(RELAY_MIST, hum < 40);
  }

  Blynk.virtualWrite(VPIN_TEMP, temp);
  Blynk.virtualWrite(VPIN_HUM, hum);
  Blynk.virtualWrite(VPIN_SOIL, soilPercent);
  Blynk.virtualWrite(VPIN_WATER, waterLevel);

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x13_tf);
  u8g2.setCursor(0,12); u8g2.printf("Temp: %.1f C", temp);
  u8g2.setCursor(0,24); u8g2.printf("Hum : %.1f %%", hum);
  u8g2.setCursor(0,36); u8g2.printf("Soil: %d %%", soilPercent);
  u8g2.setCursor(0,48); u8g2.printf("Water: %d %%", waterLevel);
  u8g2.sendBuffer();

  delay(2000);
}
