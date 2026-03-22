#define BLYNK_TEMPLATE_ID "TPML2wjUBAasI"
#define BLYNK_TEMPLATE_NAME "Air Monitor" 
#define BLYNK_DEVICE_NAME "air monitor"
#define BLYNK_AUTH_TOKEN "u4rA_jxA5T4CKtSxTwi2M2NSRXc_Crd_"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHTesp.h>

LiquidCrystal_I2C lcd1(0x27, 16, 2); // Temp + Humidity
LiquidCrystal_I2C lcd2(0x3F, 16, 2); // CO2 + Status
DHTesp dhtSensor;

char ssid[] = "Wokwi-GUEST";
char pass[] = "";

BlynkTimer timer;

// Pins
const int CO2_PIN = 34;
const int DHT_PIN = 27;

const int LED_GREEN = 25;
const int LED_YELLOW = 26;
const int LED_RED = 33;

enum AirLevel {
  NORMAL = 0,
  WARNING = 1,
  DANGER = 2
};

int readCO2ppm() {
  int raw = analogRead(CO2_PIN);
  int ppm = map(raw, 0, 4095, 400, 5000);
  return constrain(ppm, 0, 2000);
}

AirLevel getCO2Level(int ppm) {
  if (ppm < 800) return NORMAL;
  if (ppm < 1500) return WARNING;
  return DANGER;
}

AirLevel getTempLevel(float temp) {
  if (temp >= 20 && temp <= 28) return NORMAL;
  if ((temp >= 18 && temp < 20) || (temp > 28 && temp <= 32)) return WARNING;
  return DANGER;
}

AirLevel getHumidityLevel(float hum) {
  if (hum >= 40 && hum <= 60) return NORMAL;
  if ((hum >= 30 && hum < 40) || (hum > 60 && hum <= 70)) return WARNING;
  return DANGER;
}

AirLevel getOverallLevel(AirLevel t, AirLevel h, AirLevel c) {
  AirLevel overall = t;
  if (h > overall) overall = h;
  if (c > overall) overall = c;
  return overall;
}

String levelToText(AirLevel level) {
  if (level == NORMAL) return "NORMAL";
  if (level == WARNING) return "WARNING";
  return "DANGER";
}

void updateLEDs(AirLevel overall) {
  digitalWrite(LED_GREEN, overall == NORMAL ? HIGH : LOW);
  digitalWrite(LED_YELLOW, overall == WARNING ? HIGH : LOW);
  digitalWrite(LED_RED, overall == DANGER ? HIGH : LOW);
}

void printFixed(LiquidCrystal_I2C &lcd, int col, int row, String text) {
  while (text.length() < (16 - col)) {
    text += " ";
  }
  lcd.setCursor(col, row);
  lcd.print(text);
}

void showOnLCD1(float temp, float hum) {
  printFixed(lcd1, 0, 0, "Temp:" + String(temp, 1) + " C");
  printFixed(lcd1, 0, 1, "Hum :" + String(hum, 1) + " %");
}

void showOnLCD2(int co2ppm, AirLevel overall) {
  printFixed(lcd2, 0, 0, "CO2 :" + String(co2ppm));
  printFixed(lcd2, 0, 1, "Air :" + levelToText(overall));
}

void showError() {
  lcd1.clear();
  lcd2.clear();

  lcd1.setCursor(0, 0);
  lcd1.print("Sensor Error");
  lcd1.setCursor(0, 1);
  lcd1.print("Check DHT22");

  lcd2.setCursor(0, 0);
  lcd2.print("Air : DANGER");
  lcd2.setCursor(0, 1);
  lcd2.print("No DHT Data");

  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_RED, HIGH);
}

void setup() {
  Serial.begin(115200);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  analogReadResolution(12);

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_RED, LOW);

  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);

  lcd1.init();
  lcd1.backlight();
  lcd2.init();
  lcd2.backlight();

  lcd1.clear();
  lcd2.clear();

  lcd1.setCursor(0, 0);
  lcd1.print("Indoor Air");
  lcd1.setCursor(0, 1);
  lcd1.print("System Start");

  lcd2.setCursor(0, 0);
  lcd2.print("CO2 Monitor");
  lcd2.setCursor(0, 1);
  lcd2.print("Starting...");

  delay(1500);

  lcd1.clear();
  lcd2.clear();
}

void loop() {
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  float temp = data.temperature;
  float hum = data.humidity;
  int co2ppm = readCO2ppm();

  if (isnan(temp) || isnan(hum)) {
    showError();
    delay(1000);
    return;
  }

  AirLevel tempLevel = getTempLevel(temp);
  AirLevel humLevel = getHumidityLevel(hum);
  AirLevel co2Level = getCO2Level(co2ppm);
  AirLevel overall = getOverallLevel(tempLevel, humLevel, co2Level);

  updateLEDs(overall);
  showOnLCD1(temp, hum);
  showOnLCD2(co2ppm, overall);

  Serial.print("Temp: ");
  Serial.print(temp, 1);
  Serial.print(" C | Hum: ");
  Serial.print(hum, 1);
  Serial.print(" % | CO2: ");
  Serial.print(co2ppm);
  Serial.print(" ppm | Status: ");
  Serial.println(levelToText(overall));

  Blynk.virtualWrite(V0, temp);
  Blynk.virtualWrite(V1, hum);
  Blynk.virtualWrite(V2, co2ppm);
  Blynk.virtualWrite(V3, levelToText(overall));

  Blynk.run();
  delay(500);
}
