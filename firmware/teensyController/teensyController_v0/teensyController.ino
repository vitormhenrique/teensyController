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

#define DEBOUNCE 10

/*
******************************************************************
	LIBRARIES
******************************************************************
*/

#include "SPI.h"
#include "ILI9341_t3.h"
#include <Bounce2.h>
#include <BounceDescriptor.h>
//#include "color_definitions.h"

/*
******************************************************************
	PIN DEFINITION
******************************************************************
*/
#include "pin_definitions.h"

/*
******************************************************************
	CONSTANTS DEFFINITIONS
******************************************************************
*/
#define DIGITAL_BUTONS_NUMBER 5

#define FONT_HEIGHT 15
#define ITEM_HEIGHT 20
#define ITEM_LEFT_POS 24

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

status_t remoteStatus = active;

config_t remoteConfig = {false, 5,true,true};

int oldDigitalValues[DIGITAL_BUTONS_NUMBER];

unsigned long buttonPressTimeStamp;

int menuSelected = 1;
/*
******************************************************************
	OBJECTS
******************************************************************
*/
ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC); //USE HARDWARE SPI
BounceDescriptor buttons[DIGITAL_BUTONS_NUMBER] = {BounceDescriptor(),BounceDescriptor(),BounceDescriptor(),BounceDescriptor(),BounceDescriptor()}; 

void setup() {
  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  tft.setRotation(0);
  Serial.begin(9600);
  
  #ifdef DEBUG
  tft.setTextColor(ILI9341_YELLOW);
  tft.setTextSize(2);
  tft.println("Waiting for Arduino Serial Monitor...");
  while (!Serial) ; // wait for Arduino Serial Monitor
  Serial.println("Debugging...");
  delay(500);
  #endif	
  
  setupDigitalButtons();
  displayRxTxScreen();
  
}

 void loop(void) {
	 for(byte index=0;index<DIGITAL_BUTONS_NUMBER;index++){
		buttons[index].update();

		if(buttons[index].fell()){
			if(remoteStatus==active){
				if(remoteConfig.txInfo){
					tft.setTextColor(ILI9341_WHITE);
					tft.print("Button "); tft.print(buttons[index].descriptor); tft.println(" pressed.");
				}
				if(buttons[index].descriptor=="CHK"){
					buttonPressTimeStamp=millis();
				}
			}else if(remoteStatus==config){
				if(buttons[index].descriptor=="CHJ"){
					menuSelected--;
					menuSelected = constrain(menuSelected, 1, 5);
					displayConfigScreen();
				} else if(buttons[index].descriptor=="CHK"){
					if(menuSelected==5){
						remoteStatus=active;
						displayRxTxScreen();
						buttonPressTimeStamp=millis();//avoid entering confg again
					}

				}else if(buttons[index].descriptor=="CHL"){
					
				}else if(buttons[index].descriptor=="CHM"){
					menuSelected++;
					menuSelected = constrain(menuSelected, 1, 5);
					displayConfigScreen();
					
				}else if(buttons[index].descriptor=="CHN"){
					
				}
			}

		}
		if(buttons[index].rose()){
			if(remoteStatus==active){
				if(remoteConfig.txInfo && remoteStatus==active){
					tft.setTextColor(ILI9341_WHITE);
					tft.print("Button "); tft.print(buttons[index].descriptor); tft.println(" released.");
				}
			}

		}
		if(buttons[index].descriptor=="CHK" && buttons[index].read()==LOW && millis()-buttonPressTimeStamp>1000 && remoteStatus==active){
			remoteStatus = config;
			menuSelected=1;
			displayConfigScreen();
		}

			
		
	 }
}

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

void displayConfigScreen(){

	tft.fillScreen(ILI9341_BLACK);
	tft.setCursor(70, 4);
	tft.setTextSize(2);
	tft.setTextColor(ILI9341_GREEN);
	tft.print("Settings");//title
	tft.drawLine(0,20,240,20,ILI9341_WHITE);
	tft.setTextColor(ILI9341_WHITE);
	tft.setCursor(ITEM_LEFT_POS,ITEM_HEIGHT*1+(128-ITEM_HEIGHT*6));
	tft.print("Mute: "); tft.print(booleanString(remoteConfig.mute));
	
	tft.setCursor(ITEM_LEFT_POS,ITEM_HEIGHT*2+(128-ITEM_HEIGHT*6));
	tft.print("Tolerance: "); tft.print(remoteConfig.analogTolerance);

	tft.setCursor(ITEM_LEFT_POS,ITEM_HEIGHT*3+(128-ITEM_HEIGHT*6));
	tft.print("TX Info: "); tft.print(booleanString(remoteConfig.txInfo));

	tft.setCursor(ITEM_LEFT_POS,ITEM_HEIGHT*4+(128-ITEM_HEIGHT*6));
	tft.print("RX Info: "); tft.print(booleanString(remoteConfig.rxInfo));

	tft.setCursor(ITEM_LEFT_POS,ITEM_HEIGHT*5+(128-ITEM_HEIGHT*6));
	tft.print("Exit");

//tft.drawRoundRect(4, temp*ITEM_HEIGHT-1+(128-ITEM_HEIGHT*6), 120, FONT_HEIGHT+2, 5, tft.Color565(0, 255, 255));
tft.drawRoundRect(4, menuSelected*ITEM_HEIGHT-1+(128-ITEM_HEIGHT*6), 236, FONT_HEIGHT+2, 5, ILI9341_WHITE);
if(menuSelected!=5){
	tft.setTextColor(ILI9341_WHITE);
	tft.setCursor(4,ITEM_HEIGHT*menuSelected+(128-ITEM_HEIGHT*6));
	tft.print("<");
	tft.setCursor(226,ITEM_HEIGHT*menuSelected+(128-ITEM_HEIGHT*6));
	tft.print(">");
}

}

String booleanString(boolean value){
	if(value){
		return "Yes";
	}
	return "No";	
}

void setupDigitalButtons(){
//5-way Tactile Switch 1
pinMode(CHJ,INPUT_PULLUP);
buttons[0].interval(5);
buttons[0].attach(CHJ);
buttons[0].setDescriptor("CHJ");
pinMode(CHK,INPUT_PULLUP);
buttons[1].interval(5);
buttons[1].attach(CHK);
buttons[1].setDescriptor("CHK");
pinMode(CHL,INPUT_PULLUP);
buttons[2].interval(5);
buttons[2].attach(CHL);
buttons[2].setDescriptor("CHL");
pinMode(CHM,INPUT_PULLUP);
buttons[3].interval(5);
buttons[3].attach(CHM);
buttons[3].setDescriptor("CHM");
pinMode(CHN,INPUT_PULLUP);
buttons[4].interval(5);
buttons[4].attach(CHN);
buttons[4].setDescriptor("CHN"); 

}
