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
#include <EEPROMex.h>

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
#define ANALOG_INPUT_NUMBER 7

#define FONT_HEIGHT 15
#define ITEM_HEIGHT 20
#define ITEM_LEFT_POS 24
#define XBee Serial1
//TODO: PUT THIS TO THE RED BUTTON
#define BTN_CONFIG "CHK"  //DEFINE BUTTON CHANEL THAT GOES TO MENU

#define TONE_FREQUENCY 294
#define TONE_DURATION 100

// ID of the settings block
#define CONFIG_VERSION "ls1"
// Tell it where to store your config data in EEPROM
#define memoryBase 32
#define maxAllowedWrites 80

enum status_t {
  active,
  config
};

typedef struct
 {
	char version[4];
    boolean mute;
    int analogTolerance;
	boolean txInfo;
	boolean rxInfo;
 }  config_t;
 
 typedef struct
 {
    char descriptor[4];
    int analogPin;
	int analogValue;
 }  analog_t;

 /*
******************************************************************
	VARIABLES 
******************************************************************
*/

status_t remoteStatus;
String receivedData;
boolean dataComplete;
config_t configurationStorage = {CONFIG_VERSION, false, 10,true,true};

analog_t analogPins[ANALOG_INPUT_NUMBER] = {{"CHA", CHA ,0}, {"CHB", CHB,0},{"CHD", CHD,0}, 
										{"CHE", CHE,0}, {"CHG", CHG,0},{"CHH", CHH,0},{"CHI", CHI,0}};

int menuItemSelected;

bool configOk  = true;
int configAdress = 0;
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
  
  EEPROM.setMemPool(memoryBase, EEPROMSizeTeensy3);
  EEPROM.setMaxAllowedWrites(maxAllowedWrites);  
  configAdress  = EEPROM.getAddress(sizeof(configurationStorage));
  configOk = loadConfig();
  
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
  
  //if(!configOk) displayErrorWithConfig();
  
  if(configMode()){
	remoteStatus = config;
	menuItemSelected=1;
	displayConfigScreen();
  }else{
	remoteStatus = active;
	displayRxTxScreen();
  }
  getStartAnalogValues();
  if(!configurationStorage.mute) tone(BUZZER_PIN,TONE_FREQUENCY,TONE_DURATION);
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
			if(configurationStorage.rxInfo){
				if(!configurationStorage.mute) tone(BUZZER_PIN,TONE_FREQUENCY,TONE_DURATION);
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
tft.setScrollTextArea(0,21,tft.width(),tft.height()-21,ILI9341_BLACK);
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
	DISPLAY CONFIGURATION LOADING / SAVE ERROR
******************************************************************
*/
void displayErrorWithConfig(){
	tft.fillScreen(ILI9341_BLACK);
	tft.setTextSize(3);
	tft.setTextColor(ILI9341_RED);
	tft.setCursor(4, 4);
	tft.print("ERROR WHILE LOADING / SAVING CONFIG!");
	tft.setTextSize(2);
	delay(200);
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
		/*
			NOT TESTED, THIS WILL BE USEFULL FOR MENUS THAT HAVE LINES THAT DON'T FIT 1 PAGE
			CURRENTLY THE MENU ONLY HAVE FEW ITEMS AND FIT IN ONE SCREEN
		*/
		tft.fillRect(0,21,240,339,ILI9341_BLACK); 
	}
	tft.setCursor(ITEM_LEFT_POS,y);
	tft.setTextColor(color);
	switch (menuItem) {
		case 1:
			tft.print("Mute: "); tft.print(booleanString(configurationStorage.mute));
			break;
		case 2:
			tft.print("Tolerance: "); tft.print(configurationStorage.analogTolerance);
			break;
		case 3:
			tft.print("TX Info: "); tft.print(booleanString(configurationStorage.txInfo));
			break;
		case 4:
			tft.print("RX Info: "); tft.print(booleanString(configurationStorage.rxInfo));
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
			configurationStorage.mute=!configurationStorage.mute;
			break;
		case 2:
			if(shouldIncrease){
				configurationStorage.analogTolerance++;
			}else{
				configurationStorage.analogTolerance--;
			}
			break;
		case 3:
			configurationStorage.txInfo=!configurationStorage.txInfo;
			break;
		case 4:
			configurationStorage.rxInfo=!configurationStorage.rxInfo;
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
	
	//ALL ANALOG CHANELS
	for(byte i=0;i<ANALOG_INPUT_NUMBER;i++){
		int newAnalogValue = analogRead(analogPins[i].analogPin);
		if(abs(newAnalogValue-analogPins[i].analogValue)>configurationStorage.analogTolerance){
			sendChanelData(analogPins[i].descriptor,newAnalogValue);
			if(configurationStorage.txInfo){
				tft.setTextColor(ILI9341_WHITE);
				tft.print(analogPins[i].descriptor); tft.println(newAnalogValue);
			}
			analogPins[i].analogValue = newAnalogValue;
		}
	}
	
	//ALL DIGITAL CHANELS
	for(byte index=0;index<DIGITAL_BUTONS_NUMBER;index++){
		buttons[index].update();
		if(buttons[index].fell()){
			sendChanelData(buttons[index].descriptor, true);
			if(configurationStorage.txInfo){
				tft.setTextColor(ILI9341_WHITE);
				tft.print("Button "); tft.print(buttons[index].descriptor); tft.println(" pressed.");
			}
		}
		if(buttons[index].rose()){
			sendChanelData(buttons[index].descriptor, false);
			if(configurationStorage.txInfo){
				tft.setTextColor(ILI9341_WHITE);
				tft.print("Button "); tft.print(buttons[index].descriptor); tft.println(" released.");
			}	
		}
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
				drawSelectedMenu(menuItemSelected, ILI9341_BLACK); //ERASING PREVIOUS SELECTION
				menuItemSelected = constrain(--menuItemSelected, 1, 5);
				drawSelectedMenu(menuItemSelected, ILI9341_RED);	//DRAWING NEW SELECTION
			} else if(buttons[index].descriptor=="CHK"){
				if(menuItemSelected==5){
					remoteStatus=active;
					if (configOk){
						saveConfig();
					}else{
						displayErrorWithConfig();
					}
					displayRxTxScreen();
				}
			}else if(buttons[index].descriptor=="CHL"){ 
				changeMenuValue(menuItemSelected,false);
				drawMenuLine(menuItemSelected,menuItemSelected, ILI9341_WHITE);
				
			}else if(buttons[index].descriptor=="CHM"){
				drawSelectedMenu(menuItemSelected, ILI9341_BLACK);	//ERASING PREVIOUS SELECTION
				menuItemSelected = constrain(++menuItemSelected, 1, 5);
				drawSelectedMenu(menuItemSelected, ILI9341_RED);	//DRAWING NEW SELECTION
				
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
	for(byte i=0;i<ANALOG_INPUT_NUMBER;i++){
		analogPins[i].analogValue = analogRead(analogPins[i].analogPin);
	}
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

/*
******************************************************************
	CHECK IF USER IS BOOTING ON CONFIG MODE
******************************************************************
*/
boolean configMode(){
	for(byte index=0;index<DIGITAL_BUTONS_NUMBER;index++){
		buttons[index].update();
		if(buttons[index].descriptor==BTN_CONFIG && buttons[index].read()==LOW){
			return true;
		}
	}
	return false;
}

/*
******************************************************************
	LOAD CONFIGURATION FROM EEPROM
******************************************************************
*/
bool loadConfig() {
  EEPROM.readBlock(configAdress, configurationStorage);
  return (configurationStorage.version == CONFIG_VERSION);
}

/*
******************************************************************
	SAVE CONFIGURATION ON EEPROM
******************************************************************
*/
void saveConfig() {
   EEPROM.writeBlock(configAdress, configurationStorage);
}
