#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <ThingSpeak.h>

// ─── WIFI & THINGSPEAK ─────────────────────────────────────────
const char* WIFI_SSID     = "Devangi";
const char* WIFI_PASS     = "shivam@123";
unsigned long CHANNEL_ID  = 3338165;
const char* WRITE_API_KEY = "ADUQ3CMQOHMSR1H3";

// ─── SENSOR PINS ───────────────────────────────────────────────
#define DS18B20_PIN         4
#define ACS712_PIN          34

// ─── LED PINS ──────────────────────────────────────────────────
#define LED_GREEN           25
#define LED_YELLOW          26
#define LED_RED             27

// ─── BUZZER PIN ────────────────────────────────────────────────
#define BUZZER_PIN          32

// ─── ACS712 CONFIG ─────────────────────────────────────────────
#define ACS712_SENSITIVITY  0.100
#define ACS712_ZERO_OFFSET  1.884
#define ADC_RESOLUTION      4096.0
#define ADC_VREF            3.3
#define CURRENT_NOISE_CLAMP 0.05

// ─── THRESHOLDS ────────────────────────────────────────────────
#define TEMP_SAFE           30.0
#define TEMP_WARN           40.0
#define CURR_SAFE           0.5
#define CURR_WARN           1.0
#define VIB_SAFE            10.0
#define VIB_WARN            15.0

// ─── THINGSPEAK INTERVAL ───────────────────────────────────────
#define TS_INTERVAL         15000

// ─── OBJECTS ───────────────────────────────────────────────────
Adafruit_MPU6050 mpu;
OneWire oneWire(DS18B20_PIN);
DallasTemperature ds18b20(&oneWire);
WiFiClient client;
bool mpuReady  = false;
bool ds18Ready = false;
unsigned long lastUpload = 0;

// ─── ENUMS ─────────────────────────────────────────────────────
enum Status { SAFE, WARN, ALERT };

// ─── LED ───────────────────────────────────────────────────────
void setLED(Status s) {
  digitalWrite(LED_GREEN,  LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_RED,    LOW);
  if      (s == SAFE)  digitalWrite(LED_GREEN,  HIGH);
  else if (s == WARN)  digitalWrite(LED_YELLOW, HIGH);
  else                 digitalWrite(LED_RED,    HIGH);
}

void blinkLED(int pin, int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(pin, HIGH); delay(200);
    digitalWrite(pin, LOW);  delay(200);
  }
}

// ─── BUZZER ────────────────────────────────────────────────────
void buzzerPattern(Status s) {
  if (s == SAFE) {
    digitalWrite(BUZZER_PIN, LOW);       // silent
  }
  else if (s == WARN) {
    // 1 slow beep
    digitalWrite(BUZZER_PIN, HIGH); delay(200);
    digitalWrite(BUZZER_PIN, LOW);
  }
  else {
    // 3 fast beeps
    for (int i = 0; i < 3; i++) {
      digitalWrite(BUZZER_PIN, HIGH); delay(100);
      digitalWrite(BUZZER_PIN, LOW);  delay(100);
    }
  }
}

// ─── STATUS HELPERS ────────────────────────────────────────────
Status getStatus(float value, float safeLimit, float warnLimit) {
  if (value < safeLimit) return SAFE;
  if (value < warnLimit) return WARN;
  return ALERT;
}

Status overallStatus(Status t, Status c, Status v) {
  if (t == ALERT || c == ALERT || v == ALERT) return ALERT;
  if (t == WARN  || c == WARN  || v == WARN)  return WARN;
  return SAFE;
}

String statusLabel(Status s) {
  if (s == SAFE) return "SAFE ";
  if (s == WARN) return "WARN ";
  return "ALERT";
}

// ─── WIFI ──────────────────────────────────────────────────────
void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 20) {
    delay(500); Serial.print("."); tries++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi OK ✓  IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nWiFi FAILED!");
    blinkLED(LED_RED, 5);
  }
}

// ─── MPU INIT ──────────────────────────────────────────────────
void initMPU() {
  mpuReady = false;
  for (int attempt = 1; attempt <= 3; attempt++) {
    if (mpu.begin()) {
      mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
      mpu.setGyroRange(MPU6050_RANGE_250_DEG);
      mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
      Serial.println("MPU6050 OK ✓");
      mpuReady = true;
      return;
    }
    delay(500);
  }
  Serial.println("MPU6050 not ready — will retry in loop");
}

// ─── DS18B20 INIT ──────────────────────────────────────────────
void initDS18() {
  ds18Ready = false;
  ds18b20.begin();
  delay(200);
  if (ds18b20.getDeviceCount() > 0) {
    Serial.println("DS18B20 OK ✓");
    ds18Ready = true;
  } else {
    Serial.println("DS18B20 not ready — will retry in loop");
  }
}

// ─── SETUP ─────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(2000);

  // ── Pin Modes ──
  pinMode(LED_GREEN,  OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED,    OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // Startup blink + beep
  blinkLED(LED_GREEN,  2); delay(200);
  blinkLED(LED_YELLOW, 2); delay(200);
  blinkLED(LED_RED,    2); delay(200);
  digitalWrite(BUZZER_PIN, HIGH); delay(200);
  digitalWrite(BUZZER_PIN, LOW);
  Serial.println("Startup OK ✓");

  // WiFi FIRST
  connectWiFi();
  ThingSpeak.begin(client);
  Serial.println("ThingSpeak OK ✓");

  // Power settle after WiFi
  Serial.println("Stabilizing power...");
  delay(2000);

  // Sensors
  Serial.println("\n===== Sensor Init =====");
  Wire.begin(21, 22);
  Wire.setClock(100000);
  delay(500);
  initMPU();
  initDS18();
  analogReadResolution(12);
  Serial.println("ACS712  OK ✓");
  Serial.println("=======================\n");

  setLED(SAFE);
  delay(1000);
}

// ─── LOOP ──────────────────────────────────────────────────────
void loop() {
  // Reconnect WiFi if dropped
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi lost — reconnecting...");
    connectWiFi();
  }

  // Retry failed sensors
  if (!mpuReady)  initMPU();
  if (!ds18Ready) initDS18();

  // ── 1. MPU6050 ──
  float vibration = 0.0;
  if (mpuReady) {
    sensors_event_t accel, gyro, temp_mpu;
    mpu.getEvent(&accel, &gyro, &temp_mpu);
    float ax = accel.acceleration.x;
    float ay = accel.acceleration.y;
    float az = accel.acceleration.z;
    vibration = sqrt(ax*ax + ay*ay + az*az);
  }

  // ── 2. DS18B20 ──
  float tempC = -127.0;
  if (ds18Ready) {
    ds18b20.requestTemperatures();
    tempC = ds18b20.getTempCByIndex(0);
    if (tempC == -127.0) ds18Ready = false;
  }

  // ── 3. ACS712 ──
  long adcSum = 0;
  for (int i = 0; i < 500; i++) {
    adcSum += analogRead(ACS712_PIN);
    delayMicroseconds(100);
  }
  float voltage = ((adcSum / 500.0) / ADC_RESOLUTION) * ADC_VREF;
  float current = abs((voltage - ACS712_ZERO_OFFSET) / ACS712_SENSITIVITY);
  if (current < CURRENT_NOISE_CLAMP) current = 0.0;

  // ── 4. Status ──
  Status tempStatus = (!ds18Ready || tempC == -127.0) ? ALERT : getStatus(tempC, TEMP_SAFE, TEMP_WARN);
  Status currStatus = getStatus(current,   CURR_SAFE, CURR_WARN);
  Status vibStatus  = !mpuReady ? SAFE : getStatus(vibration, VIB_SAFE, VIB_WARN);
  Status overall    = overallStatus(tempStatus, currStatus, vibStatus);

  // ── 5. LED + Buzzer ──
  setLED(overall);
  buzzerPattern(overall);

  // ── 6. Serial Print ──
  Serial.println("─────────────────────────────────────");
  Serial.print("Vibration (m/s²) : ");
  if (!mpuReady) Serial.print("waiting...     ");
  else { Serial.print(vibration, 2); Serial.print("         "); }
  Serial.print("["); Serial.print(statusLabel(vibStatus)); Serial.println("]");

  Serial.print("Temperature (°C) : ");
  if (!ds18Ready || tempC == -127.0) Serial.print("waiting...     ");
  else { Serial.print(tempC, 2); Serial.print("         "); }
  Serial.print("["); Serial.print(statusLabel(tempStatus)); Serial.println("]");

  Serial.print("Current (A)      : "); Serial.print(current, 4);
  Serial.print("      ["); Serial.print(statusLabel(currStatus)); Serial.println("]");
  Serial.print(">> OVERALL        : "); Serial.println(statusLabel(overall));

  // ── 7. ThingSpeak ──
  unsigned long now = millis();
  if (now - lastUpload >= TS_INTERVAL) {
    if (mpuReady && ds18Ready) {
      lastUpload = now;
      int overallNum = (overall == SAFE) ? 0 : (overall == WARN) ? 1 : 2;

      ThingSpeak.setField(1, (float)tempC);
      ThingSpeak.setField(2, (float)vibration);
      ThingSpeak.setField(3, (float)current);
      ThingSpeak.setField(4, overallNum);

      int result = ThingSpeak.writeFields(CHANNEL_ID, WRITE_API_KEY);
      if (result == 200) Serial.println("ThingSpeak ✓ uploaded");
      else { Serial.print("ThingSpeak FAILED: "); Serial.println(result); }
    } else {
      Serial.println("ThingSpeak skipped — sensors not ready");
      lastUpload = now;
    }
  }

  Serial.println("─────────────────────────────────────");
  delay(1000);
}