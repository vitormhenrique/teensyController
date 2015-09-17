/*
******************************************************************
	DEBUG PRINT
******************************************************************
*/
//#define DEBUG //COMMENT THIS LINE DO DISABLE DEBUG PRINTING TO THE SERIAL

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

#define TONE_FREQUENCY 294
#define TONE_DURATION 100

// ID of the settings block
#define CONFIG_VERSION "ls1"
// Tell it where to store your config data in EEPROM
#define memoryBase 32
#define maxAllowedWrites 80


// FIRST 5 BUTTONS ARE ALSO USED ON THE CONFIGURATION MODE
#define CONFIG_UP 0
#define CONFIG_DOWN 3
#define CONFIG_LEFT 2
#define CONFIG_RIGHT 4
#define CONFIG_OK 1

#define CONFIG_NUM 5

#define CONFIG_ENTER_BTN 14  //DEFINE BUTTON 1 TO CHANEL THAT GOES TO MENU

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
	int frequency;
 }  config_t;
 
 typedef struct
 {
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
boolean incomeDataComplete;
config_t configurationStorage = {CONFIG_VERSION, false, 10,true,true,35};

analog_t analogPins[] = {{CHA,0}, {CHB,0},{CHD,0}, {CHE,0}, {CHG,0},{CHH,0},{CHI,0}};

byte digitalValues[DIGITAL_BUTONS_NUMBER];

int menuItemSelected;

unsigned long lastSentTime;

bool configOk  = true;
int configAdress = 0;
/*
******************************************************************
	OBJECTS
******************************************************************
*/
ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC); //USE HARDWARE SPI
Bounce buttons[DIGITAL_BUTONS_NUMBER] = {Bounce(),Bounce(),Bounce(),Bounce(),Bounce(),
												Bounce(),Bounce(),Bounce(),Bounce(),Bounce(),
												Bounce(),Bounce(),Bounce(),Bounce(),Bounce(),
												Bounce(),Bounce(),Bounce(),Bounce(),Bounce()}; 

/*
******************************************************************
	SETUP
******************************************************************
*/
void setup() {


  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  tft.setRotation(2);
  XBee.begin(9600);

  //tft.print("analog tolerance");tft.print(": ");tft.println(configurationStorage.analogTolerance);
  
  EEPROM.setMemPool(memoryBase, EEPROMSizeTeensy3);
  EEPROM.setMaxAllowedWrites(maxAllowedWrites);  
  configAdress  = EEPROM.getAddress(sizeof(configurationStorage));
  saveConfig();

  configOk = loadConfig();

  #ifdef DEBUG
      configurationStorage.frequency = 1; //ON DEBUG MODE SEND DATA SLOWWWLY = 1HZ
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
	menuItemSelected=0;
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
		if(millis()-lastSentTime>(1000/configurationStorage.frequency)){
			sendRemoteControlData();
			lastSentTime = millis();
		}
		if(incomeDataComplete){
			if(configurationStorage.rxInfo){
				if(!configurationStorage.mute) tone(BUZZER_PIN,TONE_FREQUENCY,TONE_DURATION);
				tft.setTextColor(ILI9341_BLUE);
				tft.println(receivedData);
			}
			incomeDataComplete=false;
			receivedData="";
		}
	} else if( remoteStatus==config){
		handleConfigurationState();
		//FOR NOW IF GOT DATA WHILE ON CONFIG MODE WE WILL JUST IGNORE IT
		//TODO: CREATE BUFFER AND DISPLAY THIS DATA AFTER GOING BACK TO RUNING STATE
		if(incomeDataComplete){
			incomeDataComplete=false;
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
	for(byte i=0;i==CONFIG_NUM;i++){
		drawMenuLine(i,i, ILI9341_WHITE);
	}
	drawSelectedMenu(0, ILI9341_RED);
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
void drawSelectedMenu(int itemSelected, uint16_t color){
	tft.drawRoundRect(4, (itemSelected+1)*ITEM_HEIGHT-1+(128-ITEM_HEIGHT*6), 236, FONT_HEIGHT+2, 5, color);
	if(itemSelected!=CONFIG_NUM){
		tft.setTextColor(color);
		tft.setCursor(4,ITEM_HEIGHT*(itemSelected+1)+(128-ITEM_HEIGHT*6));
		tft.print("<");
		tft.setCursor(226,ITEM_HEIGHT*(itemSelected+1)+(128-ITEM_HEIGHT*6));
		tft.print(">");
	}
}

/*
******************************************************************
	DRAW PARTICULAR MENU ITEM
******************************************************************
*/
void drawMenuLine(int menuItem, int onLine, uint16_t color){
	int y = ITEM_HEIGHT*(onLine+1)+(128-ITEM_HEIGHT*6); //START ONE SECOND ROW
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
		case 0:
			tft.print("Mute: "); tft.print(booleanString(configurationStorage.mute));
			break;
		case 1:
			tft.print("Tolerance: "); tft.print(configurationStorage.analogTolerance);
			break;
		case 2:
			tft.print("TX Info: "); tft.print(booleanString(configurationStorage.txInfo));
			break;
		case 3:
			tft.print("RX Info: "); tft.print(booleanString(configurationStorage.rxInfo));
			break;
		case 4:
			tft.print("Frequency: "); tft.print(configurationStorage.frequency);
			break;
		case 5:				
			tft.print("Exit");
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
		case 0:
			configurationStorage.mute=!configurationStorage.mute;
			break;
		case 1:
			if(shouldIncrease){
				configurationStorage.analogTolerance = constrain(configurationStorage.analogTolerance++,0,30);
			}else{
				configurationStorage.analogTolerance = constrain(configurationStorage.analogTolerance--,0,30);
			}
			break;
		case 2:
			configurationStorage.txInfo=!configurationStorage.txInfo;
			break;
		case 3:
			configurationStorage.rxInfo=!configurationStorage.rxInfo;
			break;
		case 4:
			if(shouldIncrease){
				configurationStorage.frequency = constrain(configurationStorage.frequency++,1,60);
			}else{
				configurationStorage.frequency = constrain(configurationStorage.frequency--,1,60);
			}
			break;
		case 5:
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
	for(byte index=0;index<ANALOG_INPUT_NUMBER;index++){
		int newAnalogValue = analogRead(analogPins[index].analogPin);
		if(abs(newAnalogValue-analogPins[index].analogValue)>configurationStorage.analogTolerance){
			analogPins[index].analogValue = newAnalogValue;
			if(configurationStorage.txInfo){
				tft.setTextColor(ILI9341_WHITE);
				tft.print("Analog CH"); tft.print(index); tft.print(": ");tft.println(newAnalogValue);
			}
		}
	}
	//ALL DIGITAL CHANELS
	for(byte index=0;index<DIGITAL_BUTONS_NUMBER;index++){
		buttons[index].update();
		if(configurationStorage.txInfo && buttons[index].fell()){
			tft.setTextColor(ILI9341_WHITE);
			tft.print("Button "); tft.print(index); tft.println(" pressed.");
		}
		if(configurationStorage.txInfo && buttons[index].rose()){
			tft.setTextColor(ILI9341_WHITE);
			tft.print("Button "); tft.print(index); tft.println(" released.");
		}
		digitalValues[index] =  !buttons[index].read();
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
		
			switch (index) {
				case CONFIG_UP: 
					drawSelectedMenu(menuItemSelected, ILI9341_BLACK); //ERASING PREVIOUS SELECTION
					menuItemSelected = constrain(--menuItemSelected, 0, CONFIG_NUM);
					drawSelectedMenu(menuItemSelected, ILI9341_RED);	//DRAWING NEW SELECTION
				  break;
				case CONFIG_DOWN: 
					drawSelectedMenu(menuItemSelected, ILI9341_BLACK);	//ERASING PREVIOUS SELECTION
					menuItemSelected = constrain(++menuItemSelected, 0, CONFIG_NUM);
					drawSelectedMenu(menuItemSelected, ILI9341_RED);	//DRAWING NEW SELECTION
				  break;
				case CONFIG_LEFT:
					changeMenuValue(menuItemSelected,false);
					drawMenuLine(menuItemSelected,menuItemSelected, ILI9341_WHITE);
				  break;
				case CONFIG_RIGHT:
					changeMenuValue(menuItemSelected,true);
					drawMenuLine(menuItemSelected,menuItemSelected, ILI9341_WHITE);
				  break;
				case CONFIG_OK:
					if(menuItemSelected==CONFIG_NUM){
						remoteStatus=active;
						if (configOk){
							saveConfig();
						}else{
							displayErrorWithConfig();
						}
						displayRxTxScreen();
					}
				  break;
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
	GET START DIGITAL VALUES
******************************************************************
*/
void getStartDigitalValues(){
	for(byte i=0;i<DIGITAL_BUTONS_NUMBER;i++){
		buttons[i].update();
		digitalValues[i] = !buttons[i].read();
	}
}

/*
******************************************************************
	SEND REMOTE CONTROL DATA
******************************************************************
*/

void sendRemoteControlData(){
	XBee.write(0xff);

	for(byte i=0;i<DIGITAL_BUTONS_NUMBER;i++){
		XBee.write(digitalValues[i]);
	}
	
	for(byte i=0;i<ANALOG_INPUT_NUMBER;i++){
		XBee.write(analogPins[i].analogValue);
	}
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
			incomeDataComplete = true;
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
	buttons[CONFIG_ENTER_BTN].update();
	if(buttons[CONFIG_ENTER_BTN].read()==LOW){
		return true;
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
