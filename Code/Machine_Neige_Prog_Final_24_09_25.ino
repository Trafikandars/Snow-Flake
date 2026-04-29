//Barre 12mm alu Tube PVC 100mm interieur 97

// Bitmap noir 8x8
const uint8_t blackBitmap8x8[] PROGMEM = {
  0xFF,  // Ligne 1 : 11111111
  0xFF,  // Ligne 2
  0xFF,  // Ligne 3
  0xFF,  // Ligne 4
  0xFF,  // Ligne 5
  0xFF,  // Ligne 6
  0xFF,  // Ligne 7
  0xFF   // Ligne 8
  };
const uint8_t blackBitmap24x8[] PROGMEM = {
  0xFF, 0xFF, 0xFF,  // Ligne 1 (24 pixels blancs)
  0xFF, 0xFF, 0xFF,  // Ligne 2
  0xFF, 0xFF, 0xFF,  // Ligne 3
  0xFF, 0xFF, 0xFF,  // Ligne 4
  0xFF, 0xFF, 0xFF,  // Ligne 5
  0xFF, 0xFF, 0xFF,  // Ligne 6
  0xFF, 0xFF, 0xFF,  // Ligne 7
  0xFF, 0xFF, 0xFF   // Ligne 8
  };

//___________________________________________
//
/**************  LIBRAIRIES   ***************/
//___________________________________________
#include <avr/wdt.h>   // pour le reset
//DMX
#include <DMXSerial.h>
#include <DMXSerial_avr.h>
#include <DMXSerial_megaavr.h>
//ECRAN
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C //I2C Adresse
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Moteur
#include <AccelStepper.h>
#include <MultiStepper.h>
#define DIR_PIN 6
#define STEP_PIN 5
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);
//EEPROM
#include <EEPROM.h>

//___________________________________________
//
/**************  PIN PLANNER   ***************/
//___________________________________________


//Analog IN
#define CAPTEUR_HALL A0
#define BP_UP A3
#define BP_DOWN A2
#define BP_ENTER A1
//Digital OUT
#define LED_DATA 9
#define LED_POWER 8


//___________________________________________
//
/**************  VARIABLE   ***************/
//___________________________________________


bool Erreur = false ;

//DMX
int Channel_Dmx = 1 ;           //Adresse DMX
int Channel_1_Value = 0 ;       //Valeur de du Channel vitesse
int Channel_2_Value = 0 ;       // Angle
unsigned long lastPacket = 0;   //Temps depuis le dernier paquet DMX
bool No_Data = false ;
//Moteur
int Max_Speed = 650;
int Acceleration_Max = 3000;
int Acceleration = 500;
byte Direction_Moteur = 1;
bool Home = false ;
int Vitesse_Rotation = 0;
int Vitesse_To_Home = 50;
long Target = 100 ; 
int Angle_Max = 360 ;
int Target_Max = 0;
int Sens_Rot = -1;
int Angle = 0;
//Time
unsigned long Time1 = 0;
unsigned long Time2 = 0;
//Capteur
int Capteur = 0;
bool Etat_Capteur = 0;
//Mode
bool Mode = false;  //1 = Rotation ; 0 = SHAKE

//Clavier 
#define BP_none 0     //Retour d'infos de la fonction
#define Bp_Up 1
#define Bp_Down 2
#define Bp_Enter 3
int Key_Value = 0 ;
byte Key = 0 ;

//Ecran
int Curseur = 1;
int Page = 1 ;
bool Yes_No = false ;
unsigned long dernierAffichage = 0;
const unsigned long intervalAffichage = 1000; // en ms

//___________________________________________
//
/**************  FONCTIONS   ***************/
//___________________________________________

//---RESTART
void RESET(){
  wdt_enable(WDTO_15MS); // Active le watchdog avec un timeout de 15ms
  while(1); // Boucle infinie pour forcer le reset
  }

//---EEPROM
void Eeprom_reset(){
  Channel_Dmx = 1;
  Mode = false;
  Max_Speed = 600;
  Acceleration_Max = 3000;
  Vitesse_To_Home = 100 ;
  Angle_Max = 180 ;
  EEPROM.put(0,Channel_Dmx);
  EEPROM.put(4,Mode);
  EEPROM.put(8,Max_Speed);
  EEPROM.put(12, Acceleration_Max);
  EEPROM.put(16, Vitesse_To_Home);
  EEPROM.put(20,Angle_Max);
  }//END Eeprom_reset

void Eeprom_Read(){
  EEPROM.get(0,Channel_Dmx);
  EEPROM.get(4,Mode);
  EEPROM.get(8,Max_Speed);
  EEPROM.get(12, Acceleration_Max);
  EEPROM.get(16, Vitesse_To_Home);
  EEPROM.get(20,Angle_Max);
} //END Eeprom_get

//---DMX
void Dmx_Read(){
  Channel_1_Value = DMXSerial.read(Channel_Dmx);
  Channel_2_Value = DMXSerial.read(Channel_Dmx+1);
  }

//---Lecture Analog
void Read_bouton(){
  Key_Value = 0;
  if(analogRead(BP_UP)<800){
    Key_Value = Bp_Up;
    }
  if(analogRead(BP_DOWN)<800){
    Key_Value = Bp_Down;
    }
  if(analogRead(BP_ENTER)<800){
    Key_Value = Bp_Enter;
    }
  } //END Read_Bouton()

void Read_Bp_Enter_Only(){
  Key_Value = 0;
  if(analogRead(BP_ENTER)>800){
    Key_Value = Bp_Enter;
    }
  }//END Read Enter

void Read_capteur(){
 if (analogRead(A0)<500) Etat_Capteur = true;
 else Etat_Capteur = false ;
 }

void Angle_To_Target(){
    Target_Max = (Angle_Max/1.8);
    //Target = map(Angle, 0, Angle_Max, 0, Target_Max);
    }


//---Move
void First_Home(){
  int FirstHome = 25 ;
  int Speed_First_Home = 80;
  Read_capteur();
  stepper.setSpeed(50);
  display.clearDisplay();
  display.drawLine(0,0,128,0,SSD1306_WHITE);
  display.setCursor(30, 8); display.print(F("HOME LOOSE"));
  display.setCursor(56, 18); display.print(F("!"));
  display.drawLine(0,31,128,31,SSD1306_WHITE);
  display.display();
  Read_capteur();
  stepper.setSpeed(Speed_First_Home);
     while (!Etat_Capteur) {
      Read_capteur();
      stepper.moveTo(FirstHome);
      stepper.runSpeedToPosition();
      if (stepper.distanceToGo() == 0){
          FirstHome *= -2 ;  
          }
    Time2 = millis();    
    if ((Time2-Time1)>15000) {
      Etat_Capteur = true;
      Erreur = true ;
      Key_Value = 0;
      while(Key_Value != Bp_Enter){
        Read_bouton();
        display.clearDisplay();
        display.drawLine(0,0,128,0,SSD1306_WHITE);
        display.setCursor(30, 8); display.print(F("ERROR SENSOR"));
        display.setCursor(56, 18); display.print(F("!"));
        display.drawLine(0,31,128,31,SSD1306_WHITE);
        display.display();
        }  
        //SENSOR_DISPLAY();
        }
    }//while

  }  // HOME TROUVE

void Goto_Home(){
/*  display.drawBitmap(120, 0, blackBitmap8x8, 8, 8, SSD1306_BLACK);
  display.setCursor(120, 0); display.print(F("H"));
  display.display();  */
  Home = 0;
  stepper.setSpeed(Vitesse_To_Home);
  while (!Home){
    if (analogRead(CAPTEUR_HALL)<200) Home = true ;
    else Home = false ;
    stepper.setSpeed(Vitesse_To_Home);
    stepper.runSpeed();
    }//while !Home
    Home = 0;
    delay(80);
    //si trop rapide et dépasse
    while (!Home){
      if (analogRead(CAPTEUR_HALL)<200) Home = true ;
      else Home = false ;
      stepper.setSpeed(-Vitesse_To_Home);
      stepper.runSpeed();
      }//while !Home
      Home = 0;
      stepper.setCurrentPosition(0);
  }// END HOME

//___________________________________________
//
/**************   MODE  ***************/
//___________________________________________

//----------------------------
//---    MODE ROTATION    ----
//----------------------------
void Display_Rotation(){
  display.clearDisplay();
  display.setCursor(25, 0); display.print(F("MODE Rotation"));
  display.drawLine(0,8,128,8,SSD1306_WHITE);//Ligne en dessous
  display.setCursor(0, 11); display.print(F("DMX Add.  : ")); display.setCursor(95, 11);display.print(Channel_Dmx);
  display.setCursor(0, 20); display.print(F("Max Speed : ")); display.setCursor(95, 20);display.print(Max_Speed);
  display.display();
  }

void Mode_Rotation(){
  Display_Rotation();
  Vitesse_Rotation = 0;
  Key_Value = 0;
  while(Key_Value == 0){
    Display_Whilerunning();    
    Read_Bp_Enter_Only(); Key_Value = 0;
    Channel_1_Value = DMXSerial.read(Channel_Dmx);
    if (Channel_1_Value < 25) {
        Channel_1_Value = 0;
        Goto_Home();
        Channel_1_Value = DMXSerial.read(Channel_Dmx);
        while (Channel_1_Value < 25){
              Channel_1_Value = DMXSerial.read(Channel_Dmx);
              Display_Whilerunning();
                  if(analogRead(BP_ENTER)>800){
                    Menu_Start();
                    }
              }
        }
    Vitesse_Rotation = map(Channel_1_Value,0,255,0,Max_Speed);
    stepper.setSpeed(Vitesse_Rotation);
    stepper.runSpeed();
    }
  Goto_Home();
  delay(150);
  Key_Value = 0;
  Menu_Start();
  }//END Mode Rotation

//----------------------------
//---     MODE SHAKE      ----
//----------------------------
void Display_Shake(){
  Angle_To_Target();
  display.clearDisplay();
  display.setCursor(25, 0); display.print(F("MODE SHAKE"));
  display.drawLine(0,8,128,8,SSD1306_WHITE);//Ligne en dessous
  display.setCursor(0, 11); display.print(F("DMX Add.  : ")); display.setCursor(95, 11);display.print(Channel_Dmx);
  display.setCursor(0, 20); display.print(F("Angle Max : "));display.setCursor(95, 20); display.print(Angle_Max);
  display.drawBitmap(120, 0, blackBitmap8x8, 8, 8, SSD1306_BLACK);
  display.display();
  }

void Mode_Shake(){
  Display_Shake();
  Angle_To_Target(); //definit traget_max en fonction de l'angle max
  stepper.setAcceleration(Acceleration); 
  Sens_Rot = -1;
  Key_Value = 0;
  Dmx_Read(); 
  while(Key_Value == 0){
    Read_Bp_Enter_Only();
    Display_Whilerunning();
    Dmx_Read();  
    if (Channel_1_Value < 25){
        Channel_1_Value = 0;
        Goto_Home();
        Channel_1_Value = DMXSerial.read(Channel_Dmx);
        while (Channel_1_Value < 25){
          Channel_1_Value = DMXSerial.read(Channel_Dmx);
          //Display_Whilerunning();
          if(analogRead(BP_ENTER)>800){
              Menu_Start();
              }
        }
    }// if < 25
    Acceleration = map(Channel_2_Value,0,255,20, Acceleration_Max);
    Target = map(Channel_1_Value,0,255, 0, Target_Max) * Sens_Rot;
    stepper.setAcceleration(Acceleration);       //stepper.setAcceleration(Vitesse_Rotation);
    stepper.moveTo(100 + Target);                   //stepper.moveTo(Target);
    stepper.run();
    if (stepper.distanceToGo() == 0){
      Sens_Rot = -Sens_Rot;
      }
    }//while ENTER
  Goto_Home();
  delay(150);
  Key_Value = 0;
  Menu_Start();
  }//END Mode Shake

//___________________________________________
//
/**************   DISPLAY  ***************/
//___________________________________________


 void Display_Whilerunning(){   //   FUCKING SHIIT
   Display_Data();
   if (No_Data){
      Goto_Home();
      /*stepper.setAcceleration(500);
      stepper.moveTo(0);
      stepper.runToPosition();*/
      Key_Value = 0;
      while(DMXSerial.noDataSince() > 2000) {
          Read_Bp_Enter_Only();
          display.clearDisplay();
          display.drawLine(0,0,128,0,SSD1306_WHITE);//Ligne en dessous
          display.setCursor(40, 11); display.print(F("DATA LOST"));
          display.drawLine(0,31,128,31,SSD1306_WHITE);//Ligne en dessous
          display.display();
          if (Key_Value == Bp_Enter){
          Menu_Start();
          }
         }
        if(Mode == 1) Display_Rotation();
        if(Mode == 0) Display_Shake(); 
      }
   if ((millis() - Time1) > 2000) {
    //display.display(); // lent → on le fait rarement
    Time1 = millis();
    }
   }

void Display_Data(){
 if (DMXSerial.noDataSince() < 2000) {
    digitalWrite(LED_DATA, HIGH);
    No_Data = false;
    }
  else {
    digitalWrite(LED_DATA, LOW);
    No_Data = true;
 //   stepper.setSpeed(0);
 //   stepper.setAcceleration(0);
    }   
   }

void Display_Cursor(){
    if ((Curseur%2) == 1){
      display.setCursor(120, 11);display.print(F(">"));
      }
    if ((Curseur%2) == 0 ){
      display.setCursor(120, 20);display.print(F(">"));
      }
    }//END Display_Cursor

void Cursor_Move(int Cursor_Max){
    if (Key_Value == Bp_Up){ 
      Curseur += 1;
      if (Curseur > Cursor_Max) Curseur = 1 ;
      }
    if (Key_Value == Bp_Down){ 
      Curseur -= 1; 
      if (Curseur < 1) Curseur = Cursor_Max ; 
      }
   if ((Curseur == 1) || (Curseur == 2)) {
      Page = 1;
      }
   if ((Curseur == 3) || (Curseur == 4)) {
      Page = 2;
      }
  if ((Curseur == 5) || (Curseur == 6)) {
      Page = 3;
      }
  if ((Curseur == 7) || (Curseur == 8)) {
      Page = 4;
      }
  delay(200);
  }

int Valeur_Selection (int Val_Min, int Val_Max, int Increment, int Valeur_To_Set){
      display.setCursor(0, 20); 
      display.print(F("<"));display.setCursor(120, 20); display.print(F(">"));
      display.setCursor(60, 20); display.print(Valeur_To_Set);
      if (Key_Value == Bp_Up){ 
        Valeur_To_Set += Increment; 
        if (Valeur_To_Set >Val_Max) Valeur_To_Set = Val_Min ;
        }
      if (Key_Value == Bp_Down){ 
        Valeur_To_Set -= Increment;
        if (Valeur_To_Set <Val_Min) Valeur_To_Set = Val_Max ;
        }
 return Valeur_To_Set ;        
 } //Valeur Selection


//___________________________________________
//
/**************  Menu   ***************/
//___________________________________________

void Menu_Adresse(){
  Key_Value = 0;
  while(Key_Value != Bp_Enter){
      Read_bouton();
      display.clearDisplay();
      display.setCursor(25, 0); display.print(F("ADRESSE DMX"));
      display.drawLine(0,8,128,8,SSD1306_WHITE);//Ligne en dessous
      display.setCursor(0, 20); display.print(F("<"));display.setCursor(120, 20); display.print(F(">"));
      display.setCursor(55, 20); display.print(Channel_Dmx);
      if (Key_Value == Bp_Up){ 
        Channel_Dmx += 1; 
        if (Channel_Dmx >512) Channel_Dmx = 1 ;
        }
      if (Key_Value == Bp_Down){ 
        Channel_Dmx -= 1;
        if (Channel_Dmx <1) Channel_Dmx = 512 ;
        }  
      display.display();
      delay(80);
  }//while 1
  EEPROM.put(0, Channel_Dmx );
  delay(100);
  }//END ADRESSE DMX

void Menu_Mode(){
  Key_Value = 0;
  while(Key_Value != Bp_Enter){
      Read_bouton();
      display.clearDisplay();
      display.setCursor(50, 0); display.print(F("MODE"));
      display.drawLine(0,8,128,8,SSD1306_WHITE);//Ligne en dessous
      display.setCursor(0, 20); display.print(F("<"));display.setCursor(120, 20); display.print(F(">"));
      if (Mode) {
          display.setCursor(40, 20); 
          display.print(F("Rotation"));
          }
      if (!Mode) {
         display.setCursor(45, 20); 
         display.print(F("SHAKE"));
         }
      if (Key_Value == Bp_Up){ 
        Mode = !Mode; 
        }
      if (Key_Value == Bp_Down){ 
        Mode = !Mode;
        }  
  display.display();
  delay(150);
  }//while ENTER
  EEPROM.put(4, Mode);
  delay(50);
  RESET();
  }

void Menu_Config(){
  delay(180);
  Curseur = 1 ;
  Page = 1 ;
  Key_Value = 0;
  while(Key_Value != Bp_Enter){
  display.clearDisplay();
  display.setCursor(25, 0); display.print(F("CONFIGURATION")); 
  display.drawLine(0,8,128,8,SSD1306_WHITE);
    Read_bouton();
    Cursor_Move(7);
    Display_Cursor(); //Position Selection Ligne
    if (Page == 1){
      display.setCursor(0, 11); display.print(F("1-Speed Mode ROT."));
      display.setCursor(0, 20); display.print(F("2-Speed Mode SHAKE"));
      }
  if (Page == 2){
    display.setCursor(0, 11); display.print(F("3-Angle Mode SHAKE"));
    display.setCursor(0, 20); display.print(F("4-Speed Homing"));
    }  
    if (Page == 3){
    display.setCursor(0, 11); display.print(F("5-EEPROM RESET"));
    display.setCursor(0, 20); display.print(F("6-DEBUG"));
    } 
  if (Page == 4){
    display.setCursor(0, 11); display.print(F("7-BACK"));
    display.setCursor(0, 20); display.print(F(""));
    } 

  display.display();
  }
 if (Curseur == 1) {
    Key_Value = 0;
    while(Key_Value != Bp_Enter){
      Read_bouton();
      display.clearDisplay();
      display.setCursor(15, 0); display.print(F("Max Speed ROT.")); 
      display.drawLine(0,8,128,8,SSD1306_WHITE);
      Max_Speed = Valeur_Selection (100, 600, 100, Max_Speed);
      display.display();
      delay(150);
      }//while Enter
  EEPROM.put(8,Max_Speed);
 }
 if (Curseur == 2) {
    Key_Value = 0;
    while(Key_Value != Bp_Enter){
      Read_bouton();
      display.clearDisplay();
      display.setCursor(18, 0); display.print(F("Max Speed SHAKE")); 
      display.drawLine(0,8,128,8,SSD1306_WHITE);
      Acceleration_Max = Valeur_Selection (1000, 4000, 1000, Acceleration_Max); //min, max, incrent, variable
      display.display();
      delay(150);
      }//while Enter
    EEPROM.put(12, Acceleration_Max);
    }
 if (Curseur == 3) {
    Key_Value = 0;
    while(Key_Value != Bp_Enter){
      Read_bouton();
      display.clearDisplay();
      display.setCursor(25, 0); display.print(F("Angle Max")); 
      display.drawLine(0,8,128,8,SSD1306_WHITE);
      Angle_Max = Valeur_Selection(10, 360, 10, Angle_Max); //min, max, incrent, variable
      display.display();
      delay(150);
      }//while Enter
    EEPROM.put(20,Angle_Max);
    }
 if (Curseur == 4) {
    Key_Value = 0;
    while(Key_Value != Bp_Enter){
      Read_bouton();
      display.clearDisplay();
      display.setCursor(25, 0); display.print(F("Vitesse Homing")); 
      display.drawLine(0,8,128,8,SSD1306_WHITE);
      Vitesse_To_Home = Valeur_Selection (100, 1000, 100, Vitesse_To_Home); //min, max, incrent, variable
      display.display();
      delay(150);
      }//while Enter
    EEPROM.put(16, Vitesse_To_Home);
    }
if (Curseur == 5) {
    Key_Value = 0;
    while(Key_Value != Bp_Enter){
      Read_bouton();
      display.clearDisplay();
      display.setCursor(25, 0); display.print(F("EEPROM RESET")); 
      display.drawLine(0,8,128,8,SSD1306_WHITE);
      display.setCursor(0, 20); display.print(F("<"));display.setCursor(120, 20); display.print(F(">"));
      if (Key_Value == Bp_Up) Yes_No = !Yes_No;
      if (Key_Value == Bp_Down) Yes_No = !Yes_No;
      display.setCursor(50, 20);
      if (Yes_No) display.print(F("YES"));
      if (!Yes_No) display.print(F("NO"));
      display.display();
      delay(150);
      }//while Enter
      if (Yes_No) Eeprom_reset();
      Menu_Config();
      }
      if (Curseur == 6) {
         Menu_Debug();
         }
      if (Curseur == 7) {
        Menu_Start();
        }
        
}//END CONFIGURATION


void Menu_Start(){
  delay(150);
  Curseur = 1 ;
  Page = 1 ;
  while(1){
  display.clearDisplay();
  Display_Data();
  display.setCursor(50, 0); display.print(F("MENU")); 
  display.drawLine(0,8,128,8,SSD1306_WHITE);
  if (Page == 1){
      display.setCursor(0, 11); display.print(F("1-DMX Add: ")); display.print(Channel_Dmx);
      display.setCursor(0, 20); display.print(F("2-Mode   : "));
      if(Mode == 1) display.println(F("Rotation"));
      if(Mode == 0) display.println(F("Shake")); 
      }
  if (Page == 2){
    display.setCursor(0, 11); display.print(F("3-Menu Config"));
    display.setCursor(0, 20); display.print(F("4-RESTART")); 
    }
    Read_bouton();
    if (Key_Value == Bp_Up){ 
      Curseur += 1;
      if (Curseur > 4) Curseur = 1 ;
      }
    if (Key_Value == Bp_Down){ 
      Curseur -= 1; 
      if (Curseur < 1) Curseur = 4 ; 
      }
   if ((Curseur == 1) || (Curseur == 2)) {
      Page = 1;
      }
   if ((Curseur == 3) || (Curseur == 4)) {
      Page = 2;
      }
    Display_Cursor(); //Position Selection Ligne
    display.display();
    delay(150);
    if (Key_Value == Bp_Enter){  
       if (Curseur == 1) Menu_Adresse();
       if (Curseur == 2) Menu_Mode(); 
       if (Curseur == 3) Menu_Config();
       if (Curseur == 4) Menu_RESET(); 
       }
    }//while 1
  }//END Menu_Start()

void Menu_RESET(){
 Key_Value = 0;
  while(Key_Value != Bp_Enter){
      Read_bouton();
      display.clearDisplay();
      display.setCursor(40, 0); display.print(F("RESTART"));
      display.drawLine(0,8,128,8,SSD1306_WHITE);
      display.setCursor(0, 20); display.print(F("<"));display.setCursor(120, 20); display.print(F(">"));
      if (Key_Value == Bp_Up) Yes_No = !Yes_No;
      if (Key_Value == Bp_Down) Yes_No = !Yes_No;
      display.setCursor(55, 20);
      if (Yes_No) display.print(F("YES"));
      if (!Yes_No) display.print(F("NO"));
      display.display();
      delay(150);
      }//while Enter
      if (Yes_No) RESET();
      else Menu_Start();
    }


void Menu_Debug(){
  delay(180);
  Curseur = 1 ;
  Page = 1 ;
  Key_Value = 0;
  Cursor_Move(6);
  while(Key_Value != Bp_Enter){
    Read_bouton();
    display.clearDisplay();
    display.setCursor(40, 0); display.print(F("DEBUG"));
    display.drawLine(0,8,128,8,SSD1306_WHITE);
    Cursor_Move(2);
    Display_Cursor(); //Position Selection Ligne
    if (Page == 1){
      display.setCursor(0, 11); display.print(F("1-SENSOR READ"));
      display.setCursor(0, 20); display.print(F("2-DMX READ"));
    }
  display.display();
  delay(150);
  }// while enter
  if (Curseur == 1) SENSOR_DISPLAY();
  if (Curseur == 2) DMX_DISPLAY();
}// Mode debug

void SENSOR_DISPLAY(){
  delay(200);
  Key_Value = 0 ;
  while(Key_Value != Bp_Enter){
    Read_bouton();
    display.clearDisplay();
    display.setCursor(20, 0); display.print(F("SENSOR READ"));
    display.drawLine(0,8,128,8,SSD1306_WHITE);
    display.setCursor(0, 11); display.print(analogRead(CAPTEUR_HALL));
    if (analogRead(CAPTEUR_HALL)<200) {
      display.setCursor(0, 20); display.print(F("ON"));
      }
    if (analogRead(CAPTEUR_HALL)>200) {
      display.setCursor(0, 20); display.print(F("OFF"));
      }
    display.display();
    stepper.setSpeed(200);
    stepper.runSpeed();
    delay(100);
  }//while enter
  Menu_Start();
}

void DMX_DISPLAY(){
  delay(200);
  Key_Value = 0 ;
  while(Key_Value != Bp_Enter){
    Read_bouton();
    Dmx_Read();
    display.clearDisplay();
    display.setCursor(40, 0); display.print(F("DMX READ"));
    display.drawLine(0,8,128,8,SSD1306_WHITE);
    display.setCursor(0, 11); display.print(F("Channel_1 : "));display.print(Channel_1_Value);
    display.setCursor(0, 20); display.print(F("Channel_2 : "));display.print(Channel_2_Value);
    display.display();
    delay(100);
  }//while enter
  Menu_Start();
 }
//___________________________________________
//
/**************  SETUP   ***************/
//___________________________________________


void setup() {
 //Init Pin
 pinMode(LED_POWER, OUTPUT);
 pinMode(LED_DATA, OUTPUT);
 pinMode (STEP_PIN, OUTPUT);
 pinMode (DIR_PIN, OUTPUT);
 pinMode (CAPTEUR_HALL, INPUT_PULLUP);
 pinMode (BP_UP, INPUT_PULLUP);
 pinMode (BP_DOWN, INPUT_PULLUP);
 pinMode (BP_ENTER, INPUT_PULLUP);
  //Init Lib
  Wire.begin(1);
  DMXSerial.init(DMXReceiver) ; 
  Eeprom_Read();
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  stepper.setMaxSpeed(Max_Speed);
  stepper.setAcceleration(Acceleration);
  //INIT
  digitalWrite (LED_POWER, HIGH);
  digitalWrite (LED_DATA, HIGH);
  delay(500);
  digitalWrite (LED_DATA, LOW);
  //_______AFFICHAGE START________
  display.clearDisplay();
  display.drawLine(0,1,127,1,SSD1306_WHITE);
  display.setCursor(27, 7);  display.print(F("TRAFIKANDARS")); 
  display.setCursor(27, 20); display.print(F(" SNOW FLAKE"));
  display.drawLine(0,31,127,31,SSD1306_WHITE);
  display.display();
  delay(2000);
  display.clearDisplay();
  Time1 = millis();
  for(int16_t i=0; i<max(display.width(),display.height())/2; i+=2) {
    display.drawCircle(display.width()/2, display.height()/2, i, SSD1306_WHITE);
    display.display();
    delay(1);
    }
  //____RECHERCHE PREMIER HOME AU DEMMARAGE
  First_Home();
  //____Dernier Mode
  if(Mode == 1) Mode_Rotation();
  if(Mode == 0) Mode_Shake(); 
 }// END SETUP

void loop() {
 Menu_Start();
 //Menu_Config();
 //Mode_Rotation();
 //Mode_Shake();
/************************************************************************************/
/************************************************************************************/
//                               TEST
/************************************************************************************/
/************************************************************************************/

// >>>        AFFICHAGE ANALOG 
/*
   display.clearDisplay();
   display.setCursor(0, 0); display.print(analogRead(A1));
   delay(100);
   display.display();
*/
//  >>>      RUN SPEED

 //stepper.runSpeed();

 }// END LOOP



 /*   Dmx_Read();
    display.clearDisplay();
    display.setCursor(0, 0); display.print(DMXSerial.noDataSince());
    display.display();
    delay(100);
*/


/* Vitesse_Rotation = 2000;
 Target = 200;
 while (1){
        stepper.setAcceleration(Vitesse_Rotation);//stepper.setAcceleration(Vitesse_Rotation);
        stepper.moveTo(Target);//stepper.moveTo(Target);
        stepper.run();
        }

*/
/*
const uint8_t whiteBitmap24x8[] PROGMEM = {
  0xFF, 0xFF, 0xFF,  // Ligne 1 (24 pixels blancs)
  0xFF, 0xFF, 0xFF,  // Ligne 2
  0xFF, 0xFF, 0xFF,  // Ligne 3
  0xFF, 0xFF, 0xFF,  // Ligne 4
  0xFF, 0xFF, 0xFF,  // Ligne 5
  0xFF, 0xFF, 0xFF,  // Ligne 6
  0xFF, 0xFF, 0xFF,  // Ligne 7
  0xFF, 0xFF, 0xFF   // Ligne 8
};

  /*  
    stepper.setAcceleration(200);//stepper.setAcceleration(Vitesse_Rotation);
    stepper.moveTo(Target);//stepper.moveTo(Target);
    if (stepper.distanceToGo() == 0){
        Target = -Target;
        }
    stepper.run();*/
   



/*
    display.drawBitmap(120, 0, blackBitmap8x8, 8, 8, SSD1306_BLACK);
    display.setCursor(120, 0); display.print("R");
    display.setCursor(0, 0); display.print(lastPacket);
    display.display();
    delay(3000);
    display.drawBitmap(120, 0, blackBitmap8x8, 8, 8, SSD1306_BLACK);
    display.setCursor(120, 0); display.print("Q");
    display.display();
    delay(3000);
*/
    
//Va a la position : blocant le temps d'arriver
 /* stepper.setCurrentPosition(0);
  stepper.setAcceleration(1000);
  stepper.moveTo(200);//stepper.moveTo(Target);
  stepper.runToPosition();*/










