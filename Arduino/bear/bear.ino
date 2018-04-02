#include <SoftwareSerial.h>
SoftwareSerial Serial1(2, 3); // RX, TX
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
//LiquidCrystal_I2C lcd(0x27, 16, 2);
LiquidCrystal_I2C lcd(0x3f, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

int pm25_onoffThreshold = 20;
int accuThresholdCount = 3;  //重複超過幾次才啟動
boolean disablePIR = 1; //PIR是否無作用? 若無作用則LCD的背光持續開啟

byte pinLEDred = 12;
byte pinBlue = 10;
byte pinGreen = 11;
byte pinRelay = 9;
byte pinPIR = 8;
byte pinBtnUp = 7;
byte pinBtnDown = 6;
byte pinRelayStatus = 5;  //接到IO port OUT#1, 接收來自relay的status
byte pinSwitchDevice = 4;

long pmcf10 = 0;
long pmcf25 = 0;
long pmcf100 = 0;
long pmat10 = 0;
long pmat25 = 0;
long pmat100 = 0;
unsigned int temperature = 0;
unsigned int humandity = 0;

char buf[50];
boolean deviceOnOffStatus = 0;   //relay目前應該的狀態
boolean relay_status = 0;  //relay目前實際狀態
boolean pm25Over = 0;
boolean last_pm25Over = 0;
int countOver = 0;  //累計for accuThresholdCount

void displayLCD(int yPosition, String txtMSG) {
  int xPos = (16 - txtMSG.length()) / 2;
  if (xPos < 0) xPos = 0;
  lcd.setCursor(xPos, yPosition);
  lcd.print(txtMSG);
}

void switchDevice(boolean id) {
  Serial.print("Switch to device #"+String(id));
  switch (id) {
    case 0:
      digitalWrite(pinSwitchDevice, HIGH);
      delay(50);
      break;
    case 1:
      digitalWrite(pinSwitchDevice, LOW);
      delay(50);
      break;
  }
}

void onoffDevice(boolean command) {
  deviceOnOffStatus = command;
  if (command == 0) {
    digitalWrite(pinRelay, LOW);
  } else {
    digitalWrite(pinRelay, HIGH);
  }
  delay(3000);
}

unsigned int syncRelayStatus(boolean theory, boolean actual) {
  unsigned int displayOnOff;
    
  if (theory == actual) {  
    if(deviceOnOffStatus==0) {
      displayOnOff = 0;
    } else {
      displayOnOff = 1;
    }
  }else{
    displayOnOff = 2;
  }
 
  return displayOnOff;
}

void adjustThreshold(boolean upStatus, boolean downStatus) {  
  if(upStatus==1 && downStatus==1) {
    lcd.backlight();
    if(disablePIR==0) {
      disablePIR = 1;
      displayLCD(0, "PIR Configuration");
      displayLCD(1, "  PIR disabled  ");

    }else{
      disablePIR = 0;
      displayLCD(0, "[Configuration] ");
      displayLCD(1, "  PIR enabled   ");

    }
    delay(600);
    //if(disablePIR==0) lcd.noBacklight();
  }

  if(upStatus==1 && downStatus==0) {  
    lcd.backlight();
    pm25_onoffThreshold -= 10;
    if(pm25_onoffThreshold<0) pm25_onoffThreshold=300;  
    displayLCD(0, "PM2.5 threshold ");
    displayLCD(1, "                ");
    displayLCD(1, String(pm25_onoffThreshold));
    delay(600);
    //if(disablePIR==0) lcd.noBacklight();
  }

  if(upStatus==0 && downStatus==1) {  
    lcd.backlight();
    pm25_onoffThreshold += 10;
    if(pm25_onoffThreshold>300) pm25_onoffThreshold=0;  
    displayLCD(0, "PM2.5 threshold ");
    displayLCD(1, "                ");
    displayLCD(1, String(pm25_onoffThreshold));
    delay(600);
    //if(disablePIR==0) lcd.noBacklight();
  }
}

void setup() {
  pinMode(pinLEDred, OUTPUT);
  pinMode(pinBlue, OUTPUT);
  pinMode(pinGreen, OUTPUT);
  pinMode(pinPIR, INPUT);
  pinMode(pinRelay, OUTPUT);
  digitalWrite(pinRelay, HIGH);
  pinMode(pinRelayStatus, INPUT);
  
  pinMode(pinBtnUp, INPUT);
  pinMode(pinBtnDown, INPUT);
  pinMode(pinSwitchDevice, OUTPUT);
  digitalWrite(pinSwitchDevice, HIGH);
  
  digitalWrite(pinLEDred, LOW);
  digitalWrite(pinBlue, LOW);
  digitalWrite(pinGreen, LOW);
  digitalWrite(pinLEDred, HIGH);

  
  Serial.begin(9600);
  Serial1.begin(9600);
  lcd.begin(16, 2);
  
  onoffDevice(0);
}

void loop() { // run over and over
  digitalWrite(pinLEDred, HIGH);
  digitalWrite(pinBlue, HIGH);
  digitalWrite(pinGreen, HIGH);

  boolean pirStatus = digitalRead(pinPIR);
  boolean btnUp_status = digitalRead(pinBtnUp);
  boolean btnDown_status = digitalRead(pinBtnDown);
  

  if(btnUp_status==1 or btnDown_status==1) {
    adjustThreshold(btnUp_status, btnDown_status);
    long timeStart = millis();
    while ((millis()-timeStart) <5000) {
      btnUp_status = digitalRead(pinBtnUp);
      btnDown_status = digitalRead(pinBtnDown);
        if(btnUp_status==1 or btnDown_status==1) {
          timeStart = millis();
          adjustThreshold(btnUp_status, btnDown_status);
        }
    }
  }

  // put your main code here, to run repeatedly:
  int count = 0;
  unsigned char c;
  unsigned char high;
  
  delay(500);

  while (Serial1.available()) {
    digitalWrite(pinLEDred, HIGH);
    digitalWrite(pinBlue, LOW);
    digitalWrite(pinGreen, LOW);

    c = Serial1.read();
    if ((count == 0 && c != 0x42) || (count == 1 && c != 0x4d)) {
      Serial.println("check failed");
      break;
    }
    if (count > 27) {
      Serial.println("complete");
      break;
    }    
    else if ( count == 6 || count == 24|| count == 26) {
      high = c;
    }
    else if (count == 7) {
      pmcf25 = 256 * high + c;
    }
    else if (count == 25) {
      temperature = (256 * high + c)/10 ;
      Serial.print("temperature=");
      Serial.print(temperature);
      Serial.println(" C");
    }
    else if (count == 27) {
      humandity = (256 * high + c)/10 ;
      Serial.print("humandity=");
      Serial.print(humandity);
      Serial.println(" %");
    }
    count++;
  }
  //Serial.print("PM25="); Serial.println(pmcf25);
  last_pm25Over = pm25Over;
  if (pmcf25 > pm25_onoffThreshold) {
    //Serial.println("TEST: PIR:" + String(pirStatus) + " deviceOnOffStatus=" + String(deviceOnOffStatus) + " IO1_pin=" + String(digitalRead(pinRelay)) + " pmcf25=" + String(pmcf25) + " pm25_onoffThreshold=" + String(pm25_onoffThreshold) + " btnUP:" + String(btnUp_status) + " btnDown:" + String(btnDown_status));
    pm25Over = 1;
  } else {
    pm25Over = 0;
  }

  if (last_pm25Over == pm25Over) {
    countOver += 1;
  } else {
    countOver = 0;
  }

  String relayStatusString;
  if (countOver > accuThresholdCount) {
    //Serial.println("pmcf25=" + String(pmcf25) + " pm25_onoffThreshold=" + String(pm25_onoffThreshold) + " / pm25Ov=" + String(pmcf25) + " ,pm25Over=" + String(pm25Over));
    if (pm25Over == 1) {  //超過標準, 要開relay
      
      if (deviceOnOffStatus == 0) {   //relay尚未開
        switchDevice(1);
        delay(50);
        onoffDevice(1);
        delay(50);
        switchDevice(0);
        delay(50);
      }
    } else {  //沒有超過標準, 不需要開relay
      if (deviceOnOffStatus == 1){  //如果relay有開
        onoffDevice(deviceOnOffStatus);
        
        switchDevice(1);
        delay(50);
        onoffDevice(0);
        Serial.println("deviceOnOffStatus is  On (1) now, run onoffDevice(0) ");
        delay(50);
        switchDevice(0);
        delay(50);
      }
    }
    countOver = 0;
  }

  
  deviceOnOffStatus = syncRelayStatus(deviceOnOffStatus, relay_status);
  if(deviceOnOffStatus==2) {
    relayStatusString = "Err";
    switchDevice(1);
    delay(90);
    onoffDevice(deviceOnOffStatus);
    delay(50);
    relay_status = digitalRead(pinRelayStatus);
    switchDevice(0);
    delay(50);
    deviceOnOffStatus = syncRelayStatus(deviceOnOffStatus, relay_status);
  }else{
    if(deviceOnOffStatus==0) relayStatusString = "Off";
    if(deviceOnOffStatus==1) relayStatusString = "On";
  }

  if(disablePIR==0) {
    if (pirStatus == 1) {
      lcd.backlight();
    } else {
      lcd.noBacklight();
    }
  }

  Serial.println("[" + String(pm25_onoffThreshold) + ":" + relayStatusString + "] " + String(temperature) +  "C " + String(humandity) + "%");
  displayLCD(0, "[" + String(pm25_onoffThreshold) + ":" + relayStatusString + "] " + String(temperature) +  "C " + String(humandity) + "%");
  displayLCD(1, " PM2.5: " + String(pmcf25) + "ug/m3 ");


  while (Serial1.available()) Serial1.read();
  Serial.println();
  digitalWrite(pinLEDred, HIGH);
  digitalWrite(pinBlue, HIGH);
  digitalWrite(pinGreen, HIGH);
  delay(500);
}


