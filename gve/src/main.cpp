// ORDENADO: CONTROL DE PEDAL ELECTRÓNICO PARA ARDUINO NANO

#include <Arduino.h>
#include <EEPROM.h>
#include <OLED_I2C.h>  // Asegúrate de tener esta librería instalada
#include <Wire.h>

// === DEFINICIÓN DE PINES ===
#define TML1 9
#define TML2 10
#define PINTP1 A0
#define PINTP2 A1
#define PINAPP1 A2
#define PINAPP2 A3
#define PINIDL A4
#define PINTEMACEITE A5
#define PINTEMESCAPE A6
#define PINPROGRAMER 4
int incomingByte = 0;

// === CONSTANTES ===
const int VALPWMMIN = 25;
const int VALPWMMAX = 140;
const int setpointProtected = 2;
const int rpmMax = 1023;  // Valor máximo de referencia para la barra RPM
const int xInicio = 0;
const int xFin = 126;
const int anchoUtil = 127;

// === OBJETO OLED ===
OLED myOLED(SDA, SCL,12);
extern uint8_t SmallFont[];
// === VARIABLES GLOBALES ===
int varPINIDL = 0;
int pwm = 0;
int rpm = 0;
int error = 0;

// TP
int mintp1 = 0, mintp2 = 0, maxtp1 = 0, maxtp2 = 0;
float tps1Volt = 0.0, tps2Volt = 0.0;

// APP
int minapp1 = 0, minapp2 = 0, maxapp1 = 0, maxapp2 = 0;
float app1Volt = 0.0, app2Volt = 0.0;

int dataviejaapp1 = 0, dataviejaapp2 = 0;
int dataviejatp1 = 0, dataviejatp2 = 0;

int valtp1 = 0, valtp2 = 0;
int valapp1 = 0, valapp2 = 0;
int valtp1viejo = 0;
int resultado = 0;
int setpoint = 0;
int diferencia = 0;
int pwmviejo = 0;
int address = 0;
unsigned int pwmMAX = 0;

// === BANDERAS ===
bool tempOilLimit = false;
bool tempEgrFullLimit = false;
bool tempEgrLowerLimit = false;
bool flagtpmin = false, flagtpmax = false;
bool flagappmin = false, flagappmax = false;
bool flagfinreadmax = false, flagaprendetp = false;
unsigned long tiempoOLED = 0;
const unsigned long intervaloOLED = 200;  // Actualiza cada 200ms

// === DECLARACIÓN DE FUNCIONES ===
void setup();
void loop();
void aprender(), leerEeprom(), testing();
void readtp(), readapp(), leersensor();
void readmintp(), readmaxtp(), readminapp(), readmaxapp();
void sensorestabien(), esperarRespuesta();
void mostrarDatosOLED();
void actualizarDatosOLED();
void  inicializarTextoOLED();

// === SETUP ===
void setup() {
    Wire.begin();
  Wire.setClock(400000);  // Usa modo rápido I2C
  // tu setup OLED aquí.
  myOLED.begin();
  myOLED.setFont(SmallFont);
  pinMode(TML1, OUTPUT);
  pinMode(TML2, OUTPUT);

  TCCR1A = _BV(COM1A1) | _BV(COM1B1) | _BV(WGM11);
  TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS10);
  ICR1 = 800;
  pwmMAX = ICR1;

  pinMode(PINPROGRAMER, INPUT_PULLUP);
  delay(1000);
  if (digitalRead(PINPROGRAMER) == LOW) {
    aprender();
  }
  leerEeprom();
  pwm = 0;
  setpoint = 800;
  inicializarTextoOLED();
}

// === LOOP PRINCIPAL ===
void loop() {
  testing();

    // Solo actualizar OLED cada 200ms
  if (millis() - tiempoOLED >= intervaloOLED) {
    tiempoOLED = millis();
   //mostrarDatosOLED();
   actualizarDatosOLED();
  }
  
}

// === FUNCIONES ===
void testing() {
  readapp();
  readtp();
  sensorestabien();

  app1Volt = (float)valapp1 * 5.0 / 1023.0;
  app2Volt = (float)valapp2 * 5.0 / 1023.0;
  tps1Volt = (float)valtp1 * 5.0 / 1023.0;
  tps2Volt = (float)valtp2 * 5.0 / 1023.0;

  setpoint = map(valapp2, minapp2, maxapp2, mintp1, maxtp1) + varPINIDL;
  error = setpoint - valtp1;
  int tolerancia = 2;
  int k = 9;

  if (abs(error) > tolerancia) {
    int pwm = constrain(abs(error) * k, 1, pwmMAX - 50);
    if (error > 0) {
      OCR1A = pwm;
      OCR1B = 0;
    } else {
      OCR1B = pwm;
      OCR1A = 0;
    }
  } else {
    OCR1A = 0;
    OCR1B = 0;
  }

  diferencia = error;
  pwmviejo = pwm;
}

void mostrarDatosOLED() {
  myOLED.clrScr();

  myOLED.print("RPM:", 0, 0);
  myOLED.printNumI(rpm, 35, 0);
  myOLED.print("e:", 75, 0);
  myOLED.printNumI(error, 95, 0);

  myOLED.print("A1:", 0, 10);
  myOLED.printNumF(app1Volt, 1, 25, 10);
  myOLED.print("V", 55, 10);
  myOLED.printNumI((int)(app1Volt * 100 / 5), 70, 10);
  myOLED.print("%", 100, 10);

  myOLED.print("A2:", 0, 20);
  myOLED.printNumF(app2Volt, 1, 25, 20);
  myOLED.print("V", 55, 20);
  myOLED.printNumI((int)(app2Volt * 100 / 5), 70, 20);
  myOLED.print("%", 100, 20);

  myOLED.print("T1:", 0, 30);
  myOLED.printNumF(tps1Volt, 1, 25, 30);
  myOLED.print("V", 55, 30);
  myOLED.printNumI((int)(tps1Volt * 100 / 5), 70, 30);
  myOLED.print("%", 100, 30);

  myOLED.print("T2:", 0, 40);
  myOLED.printNumF(tps2Volt, 1, 25, 40);
  myOLED.print("V", 55, 40);
  myOLED.printNumI((int)(tps2Volt * 100 / 5), 70, 40);
  myOLED.print("%", 100, 40);

  myOLED.print("PWM:", 0, 50);
  myOLED.printNumI(pwm, 35, 50);
  myOLED.print("SP:", 75, 50);
  myOLED.printNumI(setpoint, 100, 50);

  int puntos = (int)((float)rpm / rpmMax * anchoUtil);

  for (int x = xInicio; x <= xFin; x++) {
    myOLED.clrPixel(x, 63);
  }
  for (int x = xInicio; x < xInicio + puntos; x++) {
    myOLED.setPixel(x, 63);
  }
  myOLED.setPixel(0, 63);
  myOLED.setPixel(126, 63);

 myOLED.update();
}

// Las demás funciones (leerEEPROM, aprender, etc.) permanecen igual sin Serial.print
// Puedes solicitar si deseas eliminar todos los Serial por completo.

void actualizarDatosOLED() {
  myOLED.printNumI(rpm, 35, 0);
  myOLED.printNumI(error, 95, 0);

  myOLED.printNumF(app1Volt, 1, 25, 10);
  myOLED.printNumI((int)(app1Volt * 100 / 5), 70, 10);

  myOLED.printNumF(app2Volt, 1, 25, 20);
  myOLED.printNumI((int)(app2Volt * 100 / 5), 70, 20);

  myOLED.printNumF(tps1Volt, 1, 25, 30);
  myOLED.printNumI((int)(tps1Volt * 100 / 5), 70, 30);

  myOLED.printNumF(tps2Volt, 1, 25, 40);
  myOLED.printNumI((int)(tps2Volt * 100 / 5), 70, 40);

  myOLED.printNumI(pwm, 35, 50);
  myOLED.printNumI(setpoint, 100, 50);

  // Barra RPM
  int puntos = (int)((float)rpm / rpmMax * anchoUtil);

  // Limpiar barra anterior
  for (int x = xInicio; x <= xFin; x++) {
    myOLED.clrPixel(x, 63);
  }

  // Dibujar barra actual
  for (int x = xInicio; x < xInicio + puntos; x++) {
    myOLED.setPixel(x, 63);
  }

  myOLED.update();
}

void inicializarTextoOLED() {
  myOLED.clrScr();

  myOLED.print("RPM:", 0, 0);
  myOLED.print("e:", 75, 0);

  myOLED.print("A1:", 0, 10);
  myOLED.print("V", 55, 10);
  myOLED.print("%", 100, 10);

  myOLED.print("A2:", 0, 20);
  myOLED.print("V", 55, 20);
  myOLED.print("%", 100, 20);

  myOLED.print("T1:", 0, 30);
  myOLED.print("V", 55, 30);
  myOLED.print("%", 100, 30);

  myOLED.print("T2:", 0, 40);
  myOLED.print("V", 55, 40);
  myOLED.print("%", 100, 40);

  myOLED.print("PWM:", 0, 50);
  myOLED.print("SP:", 75, 50);

  myOLED.setPixel(0, 63);       // Barra delimitadora
  myOLED.setPixel(126, 63);

  myOLED.update();              // Mostrar una vez
}


void sensorestabien() {
  if (valtp1 > maxtp1 + 150 || valtp1 < mintp1 - 150 ||
      valtp2 < maxtp2 - 150 || valtp2 > mintp2 + 150 ||
      valapp1 > maxapp1 + 150 || valapp1 < minapp1 - 150 ||
      valapp2 > maxapp2 + 150 || valapp2 < minapp2 - 150) {

    while (true) {
      Serial.println("Fallo sensor");
      OCR1A = 0;
      OCR1B = 0;
      leersensor();
    }
  }
}

void leersensor() {
  readtp();
  readapp();
  Serial.print("TP1:"); Serial.print(valtp1);
  Serial.print(",TP2:"); Serial.print(valtp2);
  Serial.print(",APP1:"); Serial.print(valapp1);
  Serial.print(",APP2:"); Serial.println(valapp2);
}

void comunica() {
  readtp();
  Serial.print("TP1:"); Serial.print(valtp1);
  Serial.print(",TP2:"); Serial.print(valtp2);
  Serial.print(",PWM:"); Serial.print(pwm);
  Serial.print(",SETPOINT:"); Serial.print(setpoint);
  Serial.print(",DIFERENCIA:"); Serial.print(diferencia);
  Serial.print(",PWMVIEJO:"); Serial.print(pwmviejo);
  Serial.print(",%:"); Serial.println(map(valtp1, mintp1, maxtp1, 0, 100));

  if (Serial.available() > 0) {
    incomingByte = Serial.parseInt();
    setpoint = incomingByte;
  }
}

void readapp() {
  valapp1 = analogRead(PINAPP1);
  valapp2 = analogRead(PINAPP2);
}

void readtp() {
  valtp1 = analogRead(PINTP1);
  valtp2 = analogRead(PINTP2);
}

void readminapp() {
  readapp();
  minapp1 = valapp1;
  minapp2 = valapp2;
  dataviejaapp1 = minapp1;
  dataviejaapp2 = minapp2;
  flagappmin = true;
}

void readmaxapp() {
  readapp();
  maxapp1 = valapp1;
  maxapp2 = valapp2;
  flagappmax = true;
}

void readmintp() {
  readtp();
  mintp1 = valtp1;
  mintp2 = valtp2;
  dataviejatp1 = mintp1;
  dataviejatp2 = mintp2;
  flagtpmin = true;
  OCR1A = 0;
  OCR1B = 0;
}

void readmaxtp() {
  analogWrite(TML1, 250);
  OCR1A = pwmMAX;
  OCR1B = 0;
  delay(2000);
  readtp();
  maxtp1 = valtp1;
  maxtp2 = valtp2;
  flagtpmax = true;
  analogWrite(TML1, 0);
  OCR1A = 0;
  OCR1B = 0;
}

void esperarRespuesta() {
  while (digitalRead(PINPROGRAMER) == LOW) {}
}

void aprender() {
  address = 0;
  Serial.println("ESPERE..TP APRENDER ");
  readmintp();
  Serial.print("minTP1:"); Serial.print(mintp1);
  Serial.print(",minTP2:"); Serial.println(mintp2);
  Serial.println("LISTO APP APRENDER ");
  readminapp();
  Serial.println("Desea entrar en modo programcion? OK or NOT");
  esperarRespuesta();
  readmaxtp();
  readmaxapp();

  EEPROM.write(address, highByte(minapp1)); EEPROM.write(address + 1, lowByte(minapp1));
  EEPROM.write(address + 2, highByte(minapp2)); EEPROM.write(address + 3, lowByte(minapp2));
  EEPROM.write(address + 4, highByte(maxapp1)); EEPROM.write(address + 5, lowByte(maxapp1));
  EEPROM.write(address + 6, highByte(maxapp2)); EEPROM.write(address + 7, lowByte(maxapp2));
  EEPROM.write(address + 8, highByte(mintp1)); EEPROM.write(address + 9, lowByte(mintp1));
  EEPROM.write(address + 10, highByte(mintp2)); EEPROM.write(address + 11, lowByte(mintp2));
  EEPROM.write(address + 12, highByte(maxtp1)); EEPROM.write(address + 13, lowByte(maxtp1));
  EEPROM.write(address + 14, highByte(maxtp2)); EEPROM.write(address + 15, lowByte(maxtp2));

  Serial.println("Valor almacenado en la EEPROM. LISTO");
}

void leerEeprom() {
  address = 0;
  minapp1 = (EEPROM.read(address) << 8) | EEPROM.read(address + 1);
  minapp2 = (EEPROM.read(address + 2) << 8) | EEPROM.read(address + 3);
  maxapp1 = (EEPROM.read(address + 4) << 8) | EEPROM.read(address + 5);
  maxapp2 = (EEPROM.read(address + 6) << 8) | EEPROM.read(address + 7);
  mintp1 = (EEPROM.read(address + 8) << 8) | EEPROM.read(address + 9);
  mintp2 = (EEPROM.read(address + 10) << 8) | EEPROM.read(address + 11);
  maxtp1 = (EEPROM.read(address + 12) << 8) | EEPROM.read(address + 13);
  maxtp2 = (EEPROM.read(address + 14) << 8) | EEPROM.read(address + 15);
} 


