#include <Keypad.h>

// -------------------- KEYPAD SETUP --------------------
const byte ROWS = 4; 
const byte COLS = 4; 

char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {2, 3, 4, 5}; 
byte colPins[COLS] = {6, 7, 8, 9}; 

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// -------------------- SMART HOME PINS --------------------
#define LIGHT 10
#define FAN 11
#define DOOR 12
#define PIR A5
#define LDR A0
#define RAIN A4
#define GAS A3
#define GAS_LED A1
#define RAIN_LED A2
const int LED_PIN = 13; // Access granted LED

// Status variables
int lightStatus=0, fanStatus=0, doorStatus=0, rainStatus=0, gasStatus=0;
char option;
bool accessGranted=false;

// ---------- Function prototypes ----------
void turnLightOn();
void turnLightOff();
void turnFanOn();
void turnFanOff();
void unlockDoor();
void lockDoor();
void showStatus();
void autoMode();
void checkRain();
void checkGas();
int gasAverage();
void activateScene(char key);

// ---------- PIN DETAILS ----------
char correctPIN[] = "1234";       
char enteredPIN[5];               
byte pinIndex = 0;

// ---------- Non-blocking blinking ----------
unsigned long previousMillisRain = 0;
unsigned long previousMillisGas = 0;
unsigned long previousMillisDoor = 0;
bool rainLedState = LOW;
bool gasLedState = LOW;
bool doorLedState = LOW;
bool rainBlink=false;
bool gasBlink=false;
bool doorBlink=false;
int doorBlinkCount=0;

// -------------------- SCENE FUNCTIONS --------------------
void activateScene(char key) {
  if(key=='A'){           // Movie
    digitalWrite(LIGHT, LOW);
    analogWrite(FAN, 255);
    lockDoor();
  } else if(key=='B'){    // Study
    digitalWrite(LIGHT, HIGH);
    analogWrite(FAN, 255);
    unlockDoor();
  } else if(key=='C'){    // Sleep
    digitalWrite(LIGHT, LOW);
    analogWrite(FAN, 255);
    lockDoor();
  } else if(key=='#'){    // Ventilation
    digitalWrite(LIGHT, LOW);
    analogWrite(FAN, 255);
    digitalWrite(DOOR, HIGH);
    doorStatus=1;
  } else if(key=='D'){    // Cold
    digitalWrite(LIGHT, HIGH);
    analogWrite(FAN, 0);
    lockDoor();
  }
}

// -------------------- SETUP --------------------
void setup() {
  Serial.begin(9600);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  enteredPIN[0]='\0';

  pinMode(LIGHT, OUTPUT);
  pinMode(FAN, OUTPUT);
  pinMode(DOOR, OUTPUT);
  pinMode(PIR, INPUT);
  pinMode(GAS_LED, OUTPUT);
  pinMode(RAIN_LED, OUTPUT);

  digitalWrite(LIGHT, LOW);
  digitalWrite(FAN, LOW);
  digitalWrite(DOOR, LOW);
  digitalWrite(GAS_LED, LOW);
  digitalWrite(RAIN_LED, LOW);

  Serial.println("=== ENTER PIN TO ACCESS SMART HOME MENU ===");
}

// -------------------- LOOP --------------------
void loop() {
  char key = keypad.getKey();

  // Scene keys work anytime
  if(key) {
    if(key=='A'||key=='B'||key=='C'||key=='#'||key=='D') {
      activateScene(key);
    }
  }

  // --------- PIN ENTRY ---------
  if(!accessGranted) {
    if(key){
      Serial.print("Key pressed: "); Serial.println(key);

      if(key=='#'){
        enteredPIN[pinIndex]='\0';
        if(strcmp(enteredPIN,correctPIN)==0){
          digitalWrite(LED_PIN,HIGH);
          Serial.println("Access granted! Menu unlocked.");
          accessGranted=true;
          Serial.println("=== SMART HOME AUTOMATION MENU ===");
          Serial.println("1. Turn Light ON");
          Serial.println("2. Turn Light OFF");
          Serial.println("3. Turn Fan ON");
          Serial.println("4. Turn Fan OFF");
          Serial.println("5. Unlock Door");
          Serial.println("6. Lock Door");
          Serial.println("7. Show Status");
          Serial.println("8. Auto Mode (Sensors)");
        } else{
          digitalWrite(LED_PIN,LOW);
          Serial.println("Access denied! Try again.");
        }
        pinIndex=0;
        enteredPIN[0]='\0';
      } else if(key=='*'){
        pinIndex=0;
        enteredPIN[0]='\0';
        Serial.println("Input cleared");
      } else if(pinIndex<4){
        enteredPIN[pinIndex++]=key;
      }
    }
  }
  // --------- SMART HOME MENU ---------
  else{
    if(Serial.available()>0){
      option=Serial.read();
      switch(option){
        case '1': turnLightOn(); break;
        case '2': turnLightOff(); break;
        case '3': turnFanOn(); break;
        case '4': turnFanOff(); break;
        case '5': unlockDoor(); break;
        case '6': lockDoor(); break;
        case '7': showStatus(); break;
        case '8': autoMode(); break;
        default: Serial.println("Invalid Option! Use 1–8.");
      }
    }
    checkRain();
    checkGas();
    updateBlinking();  // Non-blocking LED updates
  }
}

// ---------- Appliance Functions ----------
void turnLightOn(){ digitalWrite(LIGHT,HIGH); lightStatus=1; Serial.println("Light ON"); }
void turnLightOff(){ digitalWrite(LIGHT,LOW); lightStatus=0; Serial.println("Light OFF"); }
void turnFanOn(){ digitalWrite(FAN,HIGH); fanStatus=1; Serial.println("Fan ON"); }
void turnFanOff(){ digitalWrite(FAN,LOW); fanStatus=0; Serial.println("Fan OFF"); }

// ---------- Door LED blinking ----------
void unlockDoor(){
  doorBlink=true; doorBlinkCount=6;  // 3 blinks
  doorStatus=1;
  Serial.println("Door Unlocked");
}
void lockDoor(){
  doorBlink=true; doorBlinkCount=6;
  doorStatus=0;
  Serial.println("Door Locked");
}

// ---------- Status ----------
void showStatus(){
  Serial.print("Light: "); Serial.println(lightStatus ? "ON":"OFF");
  Serial.print("Fan: "); Serial.println(fanStatus ? "ON":"OFF");
  Serial.print("Door: "); Serial.println(doorStatus ? "Unlocked":"Locked");
  Serial.print("Rain: "); Serial.println(rainStatus ? "Detected":"Not Detected");
  Serial.print("Gas: "); Serial.println(gasStatus ? "Detected":"Normal");
}

// ---------- Auto Mode ----------
void autoMode(){
  int ldrValue=analogRead(LDR);
  int pirValue=digitalRead(PIR);

  if(ldrValue<400){ turnLightOn(); Serial.println("Auto: Dark -> Light ON"); }
  else{ turnLightOff(); Serial.println("Auto: Bright -> Light OFF"); }

  if(pirValue==HIGH){ turnFanOn(); Serial.println("Auto: Motion Detected -> Fan ON"); }
  else{ turnFanOff(); Serial.println("Auto: No Motion -> Fan OFF"); }
}

// ---------- Rain Detection ----------
void checkRain(){
  int rainValue = analogRead(RAIN);
  
  if(rainValue <600 && !rainBlink){ // Adjust threshold
    rainStatus=1;
    Serial.println("Rain Detected!");
    rainBlink=true;
    digitalWrite(RAIN_LED, HIGH);
  } else if(rainValue > 700 && rainBlink){
    rainStatus=0;
    Serial.println("Rain Stopped.");
    rainBlink=false;
    digitalWrite(RAIN_LED, LOW);
  }
}

// ---------- Gas Detection ----------
void checkGas(){
  int gasValue=gasAverage();
  if(gasValue > 500 && !gasBlink){
    gasStatus=1;
    Serial.println("Gas Detected! ALERT!");
    gasBlink=true;
  } else if(gasValue <= 500 && gasBlink){
    gasStatus=0;
    Serial.println("Gas Normal now.");
    gasBlink=false;
    digitalWrite(GAS_LED, LOW);
  }
}

// ---------- Non-blocking LED blink updates ----------
void updateBlinking(){
  unsigned long currentMillis = millis();

  // Rain LED
  if(rainBlink){
    if(currentMillis - previousMillisRain >= 400){
      previousMillisRain=currentMillis;
      rainLedState=!rainLedState;
      digitalWrite(RAIN_LED,rainLedState);
    }
  }

  // Gas LED
  if(gasBlink){
    if(currentMillis - previousMillisGas >= 400){
      previousMillisGas=currentMillis;
      gasLedState=!gasLedState;
      digitalWrite(GAS_LED,gasLedState);
    }
  }

  // Door LED
  if(doorBlink && doorBlinkCount>0){
    if(currentMillis - previousMillisDoor >= 200){
      previousMillisDoor=currentMillis;
      doorLedState = !doorLedState;
      digitalWrite(DOOR,doorLedState);
      doorBlinkCount--;
    }
  } else doorBlink=false;
}

// ---------- Gas Averaging ----------
int gasAverage(){
  long sum=0;
  for(int i=0;i<10;i++){
    sum+=analogRead(GAS);
    delay(5);
  }
  return sum/10;
}
