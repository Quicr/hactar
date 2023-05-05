const byte led = 13;
const byte pwm = 3;

void setup() {
  Serial.begin(115200);
  pinMode(16, INPUT);
//  digitalWrite(16, LOW);

//analogWrite(pwm, 200);
}

int state = 0;
void loop() {
  Serial.println(digitalRead(16));
  delay(100);
}