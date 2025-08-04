//ORDENANDO CODIGO PARA ARDUINO NANO

#include <Arduino.h>
#include <EEPROM.h>


#define TML1 9           //PARA ABRIR LA GARGANTA
#define TML2 10           //PARA CERRAR LA GARGANTA \
                         //PINES DE ENTRADAS
#define PINTP1 A0        //SENSOR TP
#define PINTP2 A1        //SENSOR TP
#define PINAPP1 A2       // SENSOR PEDAL
#define PINAPP2 A3       // SENSOR PEDAL
#define PINIDL A4        // PINIDL PARA DEFINIR UN MINIMO RPM
#define PINTEMACEITE A5  //TEMPERATURA DE ACIETE
#define PINTEMESCAPE A6  // TEMPERATURA DE GASES DE ESCAPE
#define PINPROGRAMER 4   // pin a tierra para entrar en modo programacion
//CONSTATNTES
const int VALPWMMIN = 25;
const int VALPWMMAX = 140;
const int setpointProtected = 200;
//PINES DE SALIDA
//VARIABLES
int varPINIDL = 0;
int pwm = 0;
//TP
int mintp1 = 0;
int mintp2 = 0;
int maxtp1 = 0;
int maxtp2 = 0;
//APP
int minapp1 = 0;
int minapp2 = 0;
int maxapp1 = 0;
int maxapp2 = 0;

int dataviejaapp1 = 0;
int dataviejaapp2 = 0;
int dataviejatp1 = 0;
int dataviejatp2 = 0;

int valtp1 = 0;   //variable que almacena la lectura analógica raw
int valtp2 = 0;   //variable que almacena la lectura analógica raw
int valapp1 = 0;  //variable que almacena la lectura analógica raw
int valapp2 = 0;  //variable que almacena la lectura analógica raw
int valtp1viejo = 0;
int resultado = 0;
int incomingByte = 0;  // for incoming serial data
int setpoint = 0;
int diferencia = 0;
int pwmviejo = 0;
int address = 0;
//BANDERAS
bool tempOilLimit = false;
bool tempEgrFullLimit = false;
bool tempEgrLowerLimit = false;

bool flagtpmin = false;
bool flagtpmax = false;
bool flagappmin = false;
bool flagfinreadmax = false;
bool flagappmax = false;
bool flagaprendetp = false;
int unsigned pwmMAX=0;



// ==== DECLARACIÓN DE FUNCIONES ====
void aprender(), leerEeprom(), testing();
void readtp(), readapp(), leersensor(), comunica();
void readmintp(), readmaxtp(), readminapp(), readmaxapp();
void sensorestabien(), esperarRespuesta();

void setup() {

  //INICIO DE CONFIGURACIONES DEL  SISTEMA DEL PEDAL
  Serial.begin(115200);
  pinMode(TML1, OUTPUT);
    pinMode(TML2, OUTPUT);



  // Configurar Timer1 para Fast PWM con TOP en ICR1 (modo 14)
  TCCR1A = _BV(COM1A1) | _BV(COM1B1) | _BV(WGM11);
  TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS10); // Sin prescaler

  ICR1 = 800;  // 20kHz PWM (16MHz / 800 = 20kHz)

pwmMAX=ICR1;

  pinMode(PINPROGRAMER, INPUT_PULLUP);
  delay(1000);
  if (digitalRead(PINPROGRAMER) == LOW) {
    aprender();
  }
  leerEeprom();
  pwm = 0;
  setpoint = 800;
  //FIN DE CONFIGURACIONES DEL  SISTEMA DEL PEDAL
}

void loop() {

    testing();

}



void sensorestabien() {


  if (valtp1 > maxtp1 + 150) {
    while (true) {
      Serial.println("mayor a maxtp1");
               OCR1B =0;
      OCR1A = 0;
      leersensor();
    }
  }
  if (valtp1 < mintp1 - 150) {
    while (true) {
      Serial.println("menor a mintp1");
                OCR1B =0;
      OCR1A = 0;
      leersensor();
    }
  }
  if (valtp2 < maxtp2 - 150) {
    while (true) {
      Serial.println("mayor a maxtp2");
                  OCR1B =0;
      OCR1A = 0;
      leersensor();
    }
  }
  if (valtp2 > mintp2 + 150) {

    while (true) {
      Serial.println("menor a mintp2");
                 OCR1B =0;
      OCR1A = 0;
      leersensor();
    }
  }


  if (valapp1 > maxapp1 + 150) {
    while (true) {
                  OCR1B =0;
      OCR1A = 0;
      Serial.println("mayor a maxapp1");
      leersensor();
    }
  }
  if (valapp1 < minapp1 - 150) {
    while (true) {
            OCR1B =0;
      OCR1A = 0;
      Serial.println("menor a minapp1");
      leersensor();
    }
  }
  if (valapp2 > maxapp2 + 150) {

    while (true) {
           OCR1B = 0;
      OCR1A = 0;
      Serial.println("mayor a maxapp2");
      leersensor();
    }
  }
  if (valapp2 < minapp2 - 150) {

    while (true) {
            OCR1B = 0;
      OCR1A = 0;
      Serial.println("menor a minapp2");
      leersensor();
    }
  }
}


void testing() {
  readapp();
  readtp();
   sensorestabien();

  setpoint = map(valapp2, minapp2, maxapp2, mintp1, maxtp1);
  setpoint = varPINIDL + setpoint;

  int error = setpoint - valtp1;
  int tolerancia = 2;
  int k = 9;

  if (abs(error) > tolerancia) {
    int pwm = constrain(abs(error) * k, 1,pwmMAX-50);  // máx = ICR1 - 50 margen

    if (error > 0) {
      // Abrir mariposa
      OCR1A = pwm;
      OCR1B = 0;
    } else {
      // Cerrar mariposa
      OCR1B = pwm;
      OCR1A = 0;
    }
  }
}


void leersensor() {
  readtp();
  readapp();
  Serial.print("TP1:");
  Serial.print(valtp1);
  Serial.print(",");
  Serial.print("TP2:");
  Serial.print(valtp2);
  Serial.print(",");
  Serial.print("APP1:");
  Serial.print(valapp1);
  Serial.print(",");
  Serial.print("APP2:");
  Serial.println(valapp2);
}
void comunica() {
  readtp();
  Serial.print("TP1:");
  Serial.print(valtp1);
  Serial.print(",");
  Serial.print("TP2:");
  Serial.print(valtp2);
  Serial.print(",");
  Serial.print("PWM:");
  Serial.print(pwm);
  Serial.print(",");
  Serial.print("SETPOINT:");
  Serial.print(setpoint);
  Serial.print(",");
  Serial.print("DIFERENCIA:");
  Serial.print(diferencia);
  Serial.print(",");
  Serial.print("PWMVIEJO:");
  Serial.print(pwmviejo);
  Serial.print(",");
  Serial.print("%:");
  Serial.println(map(valtp1, mintp1, maxtp1, 0, 100));

  if (Serial.available() > 0) {
    // read the incoming byte:
    incomingByte = Serial.parseInt();

    setpoint = incomingByte;
  }
}





void readminapp() {
  readapp();
  minapp1 = valapp1;  // realizar la lectura analógica raw
  minapp2 = valapp2;  // realizar la lectura analógica raw
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
       //  OCR1B =pwmMAX;
         //     OCR1A = 0;
  //delay(2000);
  readtp();
  mintp1 = valtp1;  // realizar la lectura analógica raw
  mintp2 = valtp2;  // realizar la lectura analógica raw
  dataviejatp1 = mintp1;
  dataviejatp2 = mintp2;
  flagtpmin = true;

          OCR1A =0;
              OCR1B = 0;

}
void readmaxtp() {

  analogWrite(TML1, 250);
        OCR1A =pwmMAX;
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

void readapp() {
  valapp1 = analogRead(PINAPP1);  // realizar la lectura analógica raw
  valapp2 = analogRead(PINAPP2);  // realizar la lectura analógica raw
  //valapp2+=20;
}
void readtp() {
  valtp1 = analogRead(PINTP1);  // realizar la lectura analógica raw
  valtp2 = analogRead(PINTP2);  // realizar la lectura analógica raw
}


void leerEeprom() {

  address = 0;

  // Lee los dos bytes de la EEPROM y combínalos para obtener el valor original
  minapp1 = (EEPROM.read(address) << 8) | EEPROM.read(address + 1);

  Serial.print("Valor almacenado en la EEPROM: ");
  Serial.println(minapp1);

  minapp2 = (EEPROM.read(address + 2) << 8) | EEPROM.read(address + 3);
  Serial.print("Valor almacenado en la EEPROM: ");
  Serial.println(minapp2);

  // Lee los dos bytes de la EEPROM y combínalos para obtener el valor original
  maxapp1 = (EEPROM.read(address + 4) << 8) | EEPROM.read(address + 5);

  Serial.print("Valor almacenado en la EEPROM: ");
  Serial.println(maxapp1);

  maxapp2 = (EEPROM.read(address + 6) << 8) | EEPROM.read(address + 7);
  Serial.print("Valor almacenado en la EEPROM: ");
  Serial.println(maxapp2);


  // Lee los dos bytes de la EEPROM y combínalos para obtener el valor original
  mintp1 = (EEPROM.read(address + 8) << 8) | EEPROM.read(address + 9);

  Serial.print("Valor almacenado en la EEPROM: ");
  Serial.println(mintp1);

  mintp2 = (EEPROM.read(address + 10) << 8) | EEPROM.read(address + 11);
  Serial.print("Valor almacenado en la EEPROM: ");
  Serial.println(mintp2);

  // Lee los dos bytes de la EEPROM y combínalos para obtener el valor original
  maxtp1 = (EEPROM.read(address + 12) << 8) | EEPROM.read(address + 13);

  Serial.print("Valor almacenado en la EEPROM: ");
  Serial.println(maxtp1);

  maxtp2 = (EEPROM.read(address + 14) << 8) | EEPROM.read(address + 15);
  Serial.print("Valor almacenado en la EEPROM: ");
  Serial.println(maxtp2);

  Serial.print("minTP1:");
  Serial.print(mintp1);
  Serial.print(",");
  Serial.print("minTP2:");
  Serial.print(mintp2);
  Serial.print(",");
  Serial.print("maxTP1:");
  Serial.print(maxtp1);
  Serial.print(",");
  Serial.print("maxTP2:");
  Serial.print(maxtp2);
  Serial.println(",");
  Serial.print("minAPP1:");
  Serial.print(minapp1);
  Serial.print(",");
  Serial.print("minAPP2:");
  Serial.print(minapp2);
  Serial.print(",");
  Serial.print("maxAPP1:");
  Serial.print(maxapp1);
  Serial.print(",");
  Serial.print("maxAPP2:");
  Serial.println(maxapp2);
}
void aprender() {

  address = 0;

  Serial.println("ESPERE..TP APRENDER ");
  readmintp();
  Serial.print("minTP1:");
  Serial.print(mintp1);
  Serial.print(",");
  Serial.print("minTP2:");
  Serial.print(mintp2);
  Serial.print(",");




  Serial.println("LISTO APP APRENDER ");

  readminapp();

  Serial.println("Desea entrar en modo programcion? OK or NOT");
  esperarRespuesta();
  readmaxtp();
  readmaxapp();


  // Almacena el valor 1024 en dos bytes consecutivos
  EEPROM.write(address, highByte(minapp1));     // Almacena el byte más significativo
  EEPROM.write(address + 1, lowByte(minapp1));  // Almacena el byte menos significativo

  // Almacena el valor 1024 en dos bytes consecutivos
  EEPROM.write(address + 2, highByte(minapp2));  // Almacena el byte más significativo
  EEPROM.write(address + 3, lowByte(minapp2));   // Almacena el byte menos significativo


  EEPROM.write(address + 4, highByte(maxapp1));  // Almacena el byte más significativo
  EEPROM.write(address + 5, lowByte(maxapp1));   // Almacena el byte menos significativo

  // Almacena el valor 1024 en dos bytes consecutivos
  EEPROM.write(address + 6, highByte(maxapp2));  // Almacena el byte más significativo
  EEPROM.write(address + 7, lowByte(maxapp2));   //

  // Almacena el valor 1024 en dos bytes consecutivos
  EEPROM.write(address + 8, highByte(mintp1));  // Almacena el byte más significativo
  EEPROM.write(address + 9, lowByte(mintp1));   // Almacena el byte menos significativo

  // Almacena el valor 1024 en dos bytes consecutivos
  EEPROM.write(address + 10, highByte(mintp2));  // Almacena el byte más significativo
  EEPROM.write(address + 11, lowByte(mintp2));   // Almacena el byte menos significativo


  EEPROM.write(address + 12, highByte(maxtp1));  // Almacena el byte más significativo
  EEPROM.write(address + 13, lowByte(maxtp1));   // Almacena el byte menos significativo

  // Almacena el valor 1024 en dos bytes consecutivos
  EEPROM.write(address + 14, highByte(maxtp2));  // Almacena el byte más significativo
  EEPROM.write(address + 15, lowByte(maxtp2));   //

  Serial.println("Valor almacenado en la EEPROM.");
  Serial.println("LISTO ");
}


void esperarRespuesta() {
  while (digitalRead(PINPROGRAMER) == LOW) {
  }
}