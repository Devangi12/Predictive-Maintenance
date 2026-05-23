/*
 * ACS712 Zero Offset Calibration Sketch
 * ----------------------------------------
 * Run this BEFORE uploading the main firmware.
 * Ensure NO load is connected to ACS712 IP+ / IP- pins.
 *
 * The measured voltage at 0A is your ACS712_ZERO_OFFSET.
 * Update this value in the main firmware:
 *   #define ACS712_ZERO_OFFSET  <your measured value>
 */

#define ACS712_PIN  34

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  delay(2000);
  Serial.println("ACS712 Zero Offset Calibration");
  Serial.println("Ensure NO load is connected to ACS712!");
  Serial.println("--------------------------------------");
}

void loop() {
  long sum = 0;
  int samples = 500;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(ACS712_PIN);
    delayMicroseconds(100);
  }
  float avg     = sum / (float)samples;
  float voltage = (avg / 4096.0) * 3.3;

  Serial.print("Raw ADC: "); Serial.print(avg, 1);
  Serial.print("  |  Voltage: "); Serial.print(voltage, 4);
  Serial.println(" V  ← use this as ACS712_ZERO_OFFSET");
  delay(1000);
}
