#define VBAT_D D3

void setupvBat() {
  // Charge capacitor to turn on charge fet
  pinMode(VBAT_D, OUTPUT);
  digitalWrite(VBAT_D, LOW);  
}

float readvBat()
{
  // Charge capacitor to turn on charge fet
  digitalWrite(VBAT_D, HIGH);  
  double raw = analogRead(A0);
  // we're using 2M ohm and 249k ohm, thus the scaling factor
  double vcc = (raw/ (double)1023)*9032.1285;
  digitalWrite(VBAT_D, LOW);
  return vcc;
}

