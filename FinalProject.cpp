#include <MFRC522.h>
#include <LiquidCrystal.h> 
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <TFT_ILI9163C.h>

// Color definitions
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0  
#define WHITE   0xFFFF
#define __CS 10
#define __DC 9
TFT_ILI9163C displays = TFT_ILI9163C(__CS, 8, __DC);

#define RST_PIN A1
#define SS_PIN 10
#define EEPROM_I2C_ADDRESS 0x50
byte readCard[4];
String MasterTag = "E359D92E";	
String tagID = "";
MFRC522 mfrc522(SS_PIN, RST_PIN);
static bool isOn = false;
static bool isLost = false;
static bool isColor = false;
static int score = 0;
static int highscore = 0;
static int buttonState = 0;
static int time = 0;
static int timer = 0; 
static int numies = 0;
static int a = 1;
static int c = 3;
static int m = 4;

typedef struct task {
  int state;
  unsigned long period;
  unsigned long elapsedTime;
  int (*TickFct)(int);
} task;

int delay_gcd;
const unsigned short tasksNum = 3;
const int buzzer = A0;
const int button = 1;
task tasks[tasksNum];
const int rs = 7, en = 6, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

enum SM1_States { OFF, ON};
int SM1_Tick(int state1) {
  switch (state1) { // State transitions
    case OFF:
      tagID = "";
      getID();
      if(tagID == MasterTag){
        tone(buzzer, 500);
        delay(500);
        noTone(buzzer);
        state1 = ON;
      }
      else 
        state1 = OFF;
      break;
    case ON:
      tagID = "";
      getID();
      if(tagID == MasterTag){
        tone(buzzer, 500);
        delay(500);
        noTone(buzzer);
        state1 = OFF;
      }
      else 
        state1 = ON;
      break; 
  }
  switch (state1) { // State Action
    case OFF:
      if(isOn != false)
        displays.display(false);
      isOn = false;
      lcd.noDisplay();
      break;
    case ON:
      if(isOn != true)
        displays.display(true);
      isOn = true;
      lcd.display();
      break;
  }
  return state1;
}

enum SM2_States { INIT, REDLIGHT, GREENLIGHT, GAMEOVER, RELOAD};
int SM2_Tick(int state2) {
  //int random 
  switch (state2) { // State transitions
    case INIT:
      state2 = RELOAD;
      break;
    case RELOAD:
      score = 0;
      state2 = REDLIGHT;
      break;
    case REDLIGHT:
      numies = randomNumber();
      Serial.println(numies);
      delay(numies * 1000);
      if(isLost == true)
        state2 = GAMEOVER;
      else
        state2 = GREENLIGHT;
      break; 
    case GREENLIGHT:
      numies = randomNumber();
      Serial.println(numies);
      delay(numies * 1000);
      if(isLost == true)
        state2 = GAMEOVER;
      else
        state2 = REDLIGHT;
      break;
    case GAMEOVER:
      buttonState = digitalRead(button);
      if(buttonState == LOW)
        state2 = INIT;
      break;
  }
  switch (state2) { // State Action
    case INIT:
      break;
    case RELOAD:
      isLost = false;
      highscore = readEEPROM(0, EEPROM_I2C_ADDRESS);
      displays.fillScreen(WHITE);
      displays.drawCircle(1, 1, 1, RED);
      displays.drawRect(35, 5, 60, 120, BLACK);
      displays.drawCircle(64, 35, 26, BLACK);
      displays.drawCircle(64, 92, 26, BLACK);
      break;
    case REDLIGHT:
      isColor = false;
       for(int i = 0; i < 25; i++){
        displays.drawCircle(64, 92, i, WHITE);
      }
      for(int i = 0; i < 25; i++){
        displays.drawCircle(64, 35, i, RED);
      }
      break;
    case GREENLIGHT:
      isColor = true;
      for(int i = 0; i < 25; i++){
        displays.drawCircle(64, 35, i, WHITE);
      }
      for(int i = 0; i < 25; i++){
        displays.drawCircle(64, 92, i, GREEN);
      }
      break;
    case GAMEOVER:
          isColor = true;
          displays.fillScreen(BLACK);
          displays.setTextSize(3);
          displays.setCursor(30, 0);
          displays.print("Game");
          displays.setCursor(30, 30);
          displays.print("Over");
          displays.setCursor(15, 70);
          displays.setTextSize(2);
          displays.print("Restart?");
          if(score > highscore)
            writeEEPROM(0, score, EEPROM_I2C_ADDRESS); 
          while(buttonState != LOW){
            buttonState = digitalRead(button);
          }
      break;
  }
  return state2;
}

enum SM3_States {SCORE, LOSE};
int SM3_Tick(int state3) {
  switch (state3) { // State transitions
    case SCORE:
      if(isColor == true)
        state3 = SCORE;
      else if(isColor == false)
        state3 = LOSE;
      break;
    case LOSE:
      if(isColor == true)
        state3 = SCORE;
      else if(isColor == false)
        state3 = LOSE;
      break; 
  }
  switch (state3) { // State Action
    case SCORE:
      buttonState = digitalRead(button);
      if(buttonState == LOW && isLost == false)
        score++;
      break;
    case LOSE:
      buttonState = digitalRead(button);
      if(buttonState == LOW)
        isLost = true;
      break;
  }
  return state3;
}

void writeEEPROM(int address, byte val, int i2c_address){
  Wire.beginTransmission(i2c_address);
  Wire.write((int)(address >> 8));
  Wire.write((int)(address & 0xFF));
  Wire.write(val);
  Wire.endTransmission();
  delay(5);
}

byte readEEPROM(int address, int i2c_address){
  byte rcvData = 0xFF;
  Wire.beginTransmission(i2c_address);
  Wire.write((int)(address >> 8));
  Wire.write((int)(address & 0xFF));
  Wire.endTransmission();
  Wire.requestFrom(i2c_address, 1);
  rcvData = Wire.read();
  return rcvData;
}

int randomNumber()
{

  int times = (a * timer + c) % m;
  return abs(times);
}

void getID() 
{
  if ( ! mfrc522.PICC_IsNewCardPresent()) { 
  return;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) { 
  return;
  }
  tagID = "";
  for ( uint8_t i = 0; i < 4; i++) { 
    tagID.concat(String(mfrc522.uid.uidByte[i], HEX)); 
  }
  tagID.toUpperCase();
  mfrc522.PICC_HaltA(); 
}

void setup() {
  Wire.begin();
  SPI.begin();
  mfrc522.PCD_Init();
  lcd.begin(16, 2);
  pinMode(button, INPUT);

  displays.begin();
  displays.clearScreen();
  displays.setCursor(0,0);
  displays.setTextSize(3);
  displays.print("Welcome to");
  displays.setCursor(0,50);
  displays.print("Red &");
  displays.setCursor(0, 78);
  displays.print("Green  Light");
  delay(5000);

  unsigned char i = 0;
  tasks[i].state = ON;
  tasks[i].period = 300;
  tasks[i].elapsedTime = 0;
  tasks[i].TickFct = &SM1_Tick;
  i++;
  tasks[i].state = INIT;
  tasks[i].period = 400;
  tasks[i].elapsedTime = 0;
  tasks[i].TickFct = &SM2_Tick;
  i++;
  tasks[i].state = SCORE;
  tasks[i].period = 300;
  tasks[i].elapsedTime = 0;
  tasks[i].TickFct = &SM3_Tick;

  delay_gcd = 1000;
  Serial.begin(9600);
}

void loop() {
  lcd.setCursor(0, 0);
  lcd.print("Your Score: ");
  lcd.print(score);
  lcd.setCursor(0, 1);
  lcd.print("High score: ");
  lcd.print(highscore);

  timer += millis();
 
  unsigned char i;
  for (i = 0; i < tasksNum; ++i) {
    if ( (millis() - tasks[i].elapsedTime) >= tasks[i].period) {
      tasks[i].state = tasks[i].TickFct(tasks[i].state);
      tasks[i].elapsedTime = millis(); // Last time this task was ran
    }
  }
}

