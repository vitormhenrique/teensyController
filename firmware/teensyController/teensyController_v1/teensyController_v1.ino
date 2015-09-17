/*
******************************************************************
	DEBUG PRINT
******************************************************************
*/
#define DEBUG

#ifdef DEBUG
 #define DEBUG_PRINT(x)     Serial.print (x)
 #define DEBUG_PRINTDEC(x)     Serial.print (x, DEC)
 #define DEBUG_PRINTLN(x)  Serial.println (x)
#else
 #define DEBUG_PRINT(x)
 #define DEBUG_PRINTDEC(x)
 #define DEBUG_PRINTLN(x) 
#endif

/*
******************************************************************
	LIBRARIES
******************************************************************
*/

#include "SPI.h"
#include "ILI9341_t3.h"
#include <Bounce2.h>
#include <BounceDescriptor.h>

/*
******************************************************************
	PIN DEFINITION
******************************************************************
*/
#include "pin_definitions.h"

/*
******************************************************************
	UTILITIES
******************************************************************
*/
#include "utils.h"

/*
******************************************************************
	CONSTANTS AND DEFFINITIONS
******************************************************************
*/
#define DIGITAL_BUTONS_NUMBER 20
#define DIGITAL_BUTONS_CONFIG_NUMBER 5

#define FONT_HEIGHT 15
#define ITEM_HEIGHT 20
#define ITEM_LEFT_POS 24
#define XBee Serial1
//TODO: PUT THIS TO THE RED BUTTON
#define BTN_CONFIG "CHK"  //DEFINE BUTTON CHANEL THAT GOES TO MENU

#define TONE_FREQUENCY 294
#define TONE_DURATION 100

enum status_t {
  active,
  config
};

typedef struct
 {
    boolean mute;
    int analogTolerance;
	boolean txInfo;
	boolean rxInfo;
 }  config_t;

 /*
******************************************************************
	VARIABLES 
******************************************************************
*/

status_t remoteStatus;
String receivedData;
boolean dataComplete;
config_t remoteConfig = {false, 5,true,true};
//unsigned long buttonPressTimeStamp;
int OLD_CHA_V, OLD_CHB_V, OLD_CHD_V, OLD_CHE_V, OLD_CHG_V, OLD_CHH_V, OLD_CHI_V;




int menuItemSelected;
/*
******************************************************************
	OBJECTS
******************************************************************
*/
ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC); //USE HARDWARE SPI
BounceDescriptor buttons[DIGITAL_BUTONS_NUMBER] = {BounceDescriptor(),BounceDescriptor(),BounceDescriptor(),BounceDescriptor(),BounceDescriptor(),
												BounceDescriptor(),BounceDescriptor(),BounceDescriptor(),BounceDescriptor(),BounceDescriptor(),
												BounceDescriptor(),BounceDescriptor(),BounceDescriptor(),BounceDescriptor(),BounceDescriptor(),
												BounceDescriptor(),BounceDescriptor(),BounceDescriptor(),BounceDescriptor(),BounceDescriptor()}; 

/*
******************************************************************
	SETUP
******************************************************************
*/

void setup() {
  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  tft.setRotation(0);
  XBee.begin(9600);
  
  #ifdef DEBUG
	  Serial.begin(9600);
	  tft.setTextColor(ILI9341_YELLOW);
	  tft.setTextSize(2);
	  tft.println("Waiting for Serial Monitor...");
	  while (!Serial) ; // Wait for Arduino Serial Monitor
	  Serial.println("Debugging...");
  #endif
  
  delay(20); //ALLOW HARDWARE TO STABILIZE BEFORE RUNNING  
  setupDigitalButtons();
  if(configMode()){
	remoteStatus = config;
	menuItemSelected=1;
	displayConfigScreen();
  }else{
	remoteStatus = active;
	displayRxTxScreen();
  }
  getStartAnalogValues();
  if(!remoteConfig.mute) tone(BUZZER_PIN,TONE_FREQUENCY,TONE_DURATION);
}

/*
******************************************************************
	LOOP
******************************************************************
*/
 void loop(void) {
 
	if(remoteStatus==active){
		handleActiveState();
		if(dataComplete){
			if(remoteConfig.rxInfo){
				if(!remoteConfig.mute) tone(BUZZER_PIN,TONE_FREQUENCY,TONE_DURATION);
				tft.setTextColor(ILI9341_BLUE);
				tft.println(receivedData);
			}
			dataComplete=false;
			receivedData="";
		}
	} else if( remoteStatus==config){
		handleConfigurationState();
		//FOR NOW IF GOT DATA WHILE ON CONFIG MODE WE WILL JUST IGNORE IT
		//TODO: CREATE BUFFER AND DISPLAY THIS DATA AFTER GOING BACK TO RUNING STATE
		if(dataComplete){
			dataComplete=false;
			receivedData="";
		}
	}
}

/*
******************************************************************
	DISPLAY SCREEN WITH TX/RX INFORMATION
******************************************************************
*/
void displayRxTxScreen(){
tft.fillScreen(ILI9341_BLACK);
tft.setCursor(95, 4);
tft.setTextColor(ILI9341_GREEN); tft.setTextSize(2);
tft.println("RX/TX");
tft.drawLine(0,20,240,20,ILI9341_WHITE);
tft.enableScroll();
tft.setScrollTextArea(0,21,tft.width(),tft.height()-18,ILI9341_BLACK);
tft.setTextSize(1);
tft.setCursor(0, 21);
}

/*
******************************************************************
	DISPLAY CONFIGURATION MENU - START PAGE
******************************************************************
*/
void displayConfigScreen(){
	tft.fillScreen(ILI9341_BLACK);
	tft.setCursor(70, 4);
	tft.setTextSize(2);
	tft.setTextColor(ILI9341_GREEN);
	tft.print("Settings");//title
	tft.drawLine(0,20,240,20,ILI9341_WHITE);
	tft.setTextColor(ILI9341_WHITE);
	for(byte i=1;i<6;i++){
		drawMenuLine(i,i, ILI9341_WHITE);
	}
	drawSelectedMenu(1, ILI9341_RED);
}

/*
******************************************************************
	DRAW RECTANGULE ON SELECTED MENU
******************************************************************
*/
void drawSelectedMenu(int menuItemSelected, uint16_t color){
	tft.drawRoundRect(4, menuItemSelected*ITEM_HEIGHT-1+(128-ITEM_HEIGHT*6), 236, FONT_HEIGHT+2, 5, color);
	if(menuItemSelected!=5){
		tft.setTextColor(color);
		tft.setCursor(4,ITEM_HEIGHT*menuItemSelected+(128-ITEM_HEIGHT*6));
		tft.print("<");
		tft.setCursor(226,ITEM_HEIGHT*menuItemSelected+(128-ITEM_HEIGHT*6));
		tft.print(">");
	}
}

/*
******************************************************************
	DRAW PARTICULAR MENU ITEM
******************************************************************
*/
void drawMenuLine(int menuItem, int onLine, uint16_t color){
	int y = ITEM_HEIGHT*onLine+(128-ITEM_HEIGHT*6);
	if(menuItem==onLine){
		tft.fillRect(ITEM_LEFT_POS, y, 202, FONT_HEIGHT, ILI9341_BLACK);
	}else{
	//TODO: CLEAR THE ENTIRE MENU SECTION TO DRAW NEW LINES WITH OFFSET
	}
	tft.setCursor(ITEM_LEFT_POS,y);
	tft.setTextColor(color);
	switch (menuItem) {
		case 1:
			tft.print("Mute: "); tft.print(booleanString(remoteConfig.mute));
			break;
		case 2:
			tft.print("Tolerance: "); tft.print(remoteConfig.analogTolerance);
			break;
		case 3:
			tft.print("TX Info: "); tft.print(booleanString(remoteConfig.txInfo));
			break;
		case 4:
			tft.print("RX Info: "); tft.print(booleanString(remoteConfig.rxInfo));
			break;
		case 5:
			tft.print("Exit");
			break;
		default: 
			break;
	}
}

/*
******************************************************************
	CHANGE MENU VALUE
******************************************************************
*/
void changeMenuValue(int menuItem, boolean shouldIncrease){
	switch (menuItem) {
		case 1:
			remoteConfig.mute=!remoteConfig.mute;
			break;
		case 2:
			if(shouldIncrease){
				remoteConfig.analogTolerance++;
			}else{
				remoteConfig.analogTolerance--;
			}
			break;
		case 3:
			remoteConfig.txInfo=!remoteConfig.txInfo;
			break;
		case 4:
			remoteConfig.rxInfo=!remoteConfig.rxInfo;
			break;
		case 5:
			break;
		default:
			break;
	}
}

/*
******************************************************************
	HANDLE ACTIVE STATE
******************************************************************
*/
void handleActiveState(){
	int analogValue;
	/*

	// CHANEL A
	 analogValue = analogRead(CHA);
	analogValue = abs(analogValue-OLD_CHA_V)>remoteConfig.analogTolerance ? analogValue : OLD_CHA_V;
	sendChanelData("CHA",analogValue);
	if(remoteConfig.txInfo){
		tft.setTextColor(ILI9341_WHITE);
		tft.print("CHA: "); tft.println(analogValue);
	}
	
	 // CHANEL B
	 analogValue = analogRead(CHB);
	 analogValue = abs(analogValue-OLD_CHB_V)>remoteConfig.analogTolerance ? analogValue : OLD_CHB_V;
	 sendChanelData("CHB",analogValue);
	 if(remoteConfig.txInfo){
	 	tft.setTextColor(ILI9341_WHITE);
	 	tft.print("CHB: "); tft.println(analogValue);
	 }
	
	 // CHANEL D
	 analogValue = analogRead(CHD);
	 analogValue = abs(analogValue-OLD_CHD_V)>remoteConfig.analogTolerance ? analogValue : OLD_CHD_V;
	 sendChanelData("CHD",analogValue);
	 if(remoteConfig.txInfo){
	 	tft.setTextColor(ILI9341_WHITE);
	 	tft.print("CHD: "); tft.println(analogValue);
	 }
	
	 // CHANEL E
	 analogValue = analogRead(CHE);
	 analogValue = abs(analogValue-OLD_CHE_V)>remoteConfig.analogTolerance ? analogValue : OLD_CHE_V;
	 sendChanelData("CHE",analogValue);
	 if(remoteConfig.txInfo){
	 	tft.setTextColor(ILI9341_WHITE);
	 	tft.print("CHE: "); tft.println(analogValue);
	 }
	
	 // CHANEL G
	 analogValue = analogRead(CHG);
	 analogValue = abs(analogValue-OLD_CHG_V)>remoteConfig.analogTolerance ? analogValue : OLD_CHG_V;
	 sendChanelData("CHG",analogValue);
	 if(remoteConfig.txInfo){
	 	tft.setTextColor(ILI9341_WHITE);
	 	tft.print("CHG: "); tft.println(analogValue);
	 }
	
	 // CHANEL H
	 analogValue = analogRead(CHH);
	 analogValue = abs(analogValue-OLD_CHH_V)>remoteConfig.analogTolerance ? analogValue : OLD_CHH_V;
	 sendChanelData("CHH",analogValue);
	 if(remoteConfig.txInfo){
	 	tft.setTextColor(ILI9341_WHITE);
	 	tft.print("CHH: "); tft.println(analogValue);
	 } */
	
	 // CHANEL I
	 analogValue = analogRead(CHI);
	 if(abs(analogValue-OLD_CHI_V)>remoteConfig.analogTolerance){
 		sendChanelData("CHI",analogValue);
		if(remoteConfig.txInfo){
		 	tft.setTextColor(ILI9341_WHITE);
		 	tft.print("CHI: "); tft.println(analogValue);
		}
		OLD_CHI_V = analogValue;
	 }

	
	//ALL DIGITAL CHANELS
	for(byte index=0;index<DIGITAL_BUTONS_NUMBER;index++){
		buttons[index].update();
		if(buttons[index].fell()){
			sendChanelData(buttons[index].descriptor, true);
			if(remoteConfig.txInfo){
				tft.setTextColor(ILI9341_WHITE);
				tft.print("Button "); tft.print(buttons[index].descriptor); tft.println(" pressed.");
			}
			/*if(buttons[index].descriptor==BTN_CONFIG){
				buttonPressTimeStamp=millis();
			}*/	
		}
		if(buttons[index].rose()){
			sendChanelData(buttons[index].descriptor, false);
			if(remoteConfig.txInfo){
				tft.setTextColor(ILI9341_WHITE);
				tft.print("Button "); tft.print(buttons[index].descriptor); tft.println(" released.");
			}	
		}
		/*if(buttons[index].descriptor==BTN_CONFIG && buttons[index].read()==LOW && millis()-buttonPressTimeStamp>1000){
			remoteStatus = config;
			menuItemSelected=1;
			displayConfigScreen();
		}*/
	}
}

/*
******************************************************************
	HANDLE CONFIGURATION STATE
******************************************************************
*/
void handleConfigurationState(){
	//Handle 6 digital Buttons for the configuration
	for(byte index=0;index<DIGITAL_BUTONS_CONFIG_NUMBER;index++){
		buttons[index].update();
		if(buttons[index].fell()){
			if(buttons[index].descriptor=="CHJ"){
				drawSelectedMenu(menuItemSelected, ILI9341_BLACK);
				menuItemSelected--;
				menuItemSelected = constrain(menuItemSelected, 1, 5);
				drawSelectedMenu(menuItemSelected, ILI9341_RED);
			} else if(buttons[index].descriptor=="CHK"){
				if(menuItemSelected==5){
					remoteStatus=active;
					displayRxTxScreen();
					//buttonPressTimeStamp=millis();//avoid entering configuration mode again
				}
			}else if(buttons[index].descriptor=="CHL"){ 
				changeMenuValue(menuItemSelected,false);
				drawMenuLine(menuItemSelected,menuItemSelected, ILI9341_WHITE);
				
			}else if(buttons[index].descriptor=="CHM"){
				drawSelectedMenu(menuItemSelected, ILI9341_BLACK);
				menuItemSelected++;
				menuItemSelected = constrain(menuItemSelected, 1, 5);
				drawSelectedMenu(menuItemSelected, ILI9341_RED);
				
			}else if(buttons[index].descriptor=="CHN"){
				changeMenuValue(menuItemSelected,true);
				drawMenuLine(menuItemSelected,menuItemSelected, ILI9341_WHITE);
			}
		}
	}
}

/*
******************************************************************
	GET START ANALOG VALUES
******************************************************************
*/
void getStartAnalogValues(){
	OLD_CHA_V = analogRead(CHA);
	OLD_CHB_V = analogRead(CHB);
	OLD_CHD_V = analogRead(CHD);
	OLD_CHE_V = analogRead(CHE);
	OLD_CHG_V = analogRead(CHG);
	OLD_CHH_V = analogRead(CHH);
	OLD_CHI_V = analogRead(CHI);
}

/*
******************************************************************
	SEND CHANEL DATA
******************************************************************
*/

void sendChanelData(String chanel, String data){
	XBee.print(chanel + ":" + data + '\r');
}

/*
******************************************************************
	GOT DATA
******************************************************************
*/
void serialEvent1() {
	while (XBee.available()) {
		// get the new byte:
		char inChar = (char)XBee.read(); 
		//if the incoming character is a CARRIAGE RETURN, set a flag
		//so the main loop can do something about it:
		if (inChar == '\r') {
			dataComplete = true;
			DEBUG_PRINTLN(receivedData);
		} else{
			receivedData += inChar;
		}
	}
}


boolean configMode(){
	for(byte index=0;index<DIGITAL_BUTONS_NUMBER;index++){
		buttons[index].update();
		if(buttons[index].descriptor==BTN_CONFIG && buttons[index].read()==LOW){
			return true;
		}
	}
	return false;
}
