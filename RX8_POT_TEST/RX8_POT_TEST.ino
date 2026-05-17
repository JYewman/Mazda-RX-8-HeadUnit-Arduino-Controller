/*
 * RX-8 Hood Potentiometer Test
 *
 * Continuously prints the hood-position pot reading on A5, plus the running
 * min/max seen since boot. Use to verify pot wiring/polarity and confirm
 * the full mechanical travel range before flashing the controller firmware.
 *
 * Manually sweep the hood from fully open to fully closed while watching
 * the serial output. The min/max values give you the actual ADC range
 * this specific pot/install produces — plug those numbers (with a small
 * safety margin) into HOODOPENEDVALUE / HOODCLOSEDVALUE in RX8_SN_CTRL.ino.
 *
 * Serial Monitor: 115200 baud.
 */

const uint8_t HOODPOSPIN = A5;

int minVal = 1023;
int maxVal = 0;

void setup()
{
  Serial.begin(115200);
  Serial.println(F("Hood pot test — move the hood and watch the range"));
}

void loop()
{
  int v = analogRead(HOODPOSPIN);
  if (v < minVal) minVal = v;
  if (v > maxVal) maxVal = v;

  Serial.print(F("value="));
  Serial.print(v);
  Serial.print(F("  min="));
  Serial.print(minVal);
  Serial.print(F("  max="));
  Serial.print(maxVal);
  Serial.print(F("  span="));
  Serial.println(maxVal - minVal);

  delay(3);
}
