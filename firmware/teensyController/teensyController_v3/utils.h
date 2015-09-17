#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif


/*
******************************************************************
	BOOLEAN TO STRING
******************************************************************
*/
String booleanString(boolean value){
	if(value) return "Yes";
		
	return "No";	
}

/*
******************************************************************
	SETUP ROUTINE FOR BUTTONS
******************************************************************
*/


extern Bounce buttons[];

void setupDigitalButtons(){
	//5-way Tactile Switch 1
	pinMode(CHJ,INPUT_PULLUP);
	buttons[0].interval(5);
	buttons[0].attach(CHJ); // UP
	pinMode(CHK,INPUT_PULLUP);
	buttons[1].interval(5);
	buttons[1].attach(CHK);// CENTER
	pinMode(CHL,INPUT_PULLUP);
	buttons[2].interval(5);
	buttons[2].attach(CHL); // LEFT
	pinMode(CHM,INPUT_PULLUP);
	buttons[3].interval(5);
	buttons[3].attach(CHM);// DOWN
	pinMode(CHN,INPUT_PULLUP);
	buttons[4].interval(5);
	buttons[4].attach(CHN); // RIGHT
	
	//5-way Tactile Switch 2
	pinMode(CHO,INPUT_PULLUP);
	buttons[5].interval(5);
	buttons[5].attach(CHO); // UP
	pinMode(CHP,INPUT_PULLUP);
	buttons[6].interval(5);
	buttons[6].attach(CHP);// CENTER
	pinMode(CHQ,INPUT_PULLUP);
	buttons[7].interval(5);
	buttons[7].attach(CHQ); // LEFT
	pinMode(CHR,INPUT_PULLUP);
	buttons[8].interval(5);
	buttons[8].attach(CHR); // DOWN
	pinMode(CHS,INPUT_PULLUP);
	buttons[9].interval(5);
	buttons[9].attach(CHS); // RIGHT
	
	//Toggle switch 1
	pinMode(CHT,INPUT_PULLUP);
	buttons[10].interval(5);
	buttons[10].attach(CHT); // 

	//Toggle switch 2
	pinMode(CHU,INPUT_PULLUP);
	buttons[11].interval(5);
	buttons[11].attach(CHU); // 

	//Toggle switch 3
	pinMode(CHV,INPUT_PULLUP);
	buttons[12].interval(5);
	buttons[12].attach(CHV); // 

	//Toggle switch 4
	pinMode(CHX,INPUT_PULLUP);
	buttons[13].interval(5);
	buttons[13].attach(CHX); // 

	//Button 1
	pinMode(CHY,INPUT_PULLUP);
	buttons[14].interval(5);
	buttons[14].attach(CHY); // 

	//Button 2
	pinMode(CHW,INPUT_PULLUP);
	buttons[15].interval(5);
	buttons[15].attach(CHW); // 

	//Button 3
	pinMode(CHZ,INPUT_PULLUP);
	buttons[16].interval(5);
	buttons[16].attach(CHZ); // 

	//Button 4
	pinMode(CH0,INPUT_PULLUP);
	buttons[17].interval(5);
	buttons[17].attach(CH0); // 
	
	//Joystick 1
	pinMode(CHC,INPUT_PULLUP);
	buttons[18].interval(5);
	buttons[18].attach(CHC);// CENTER
	pinMode(CHA,INPUT);				  // HORIZONTAL
	pinMode(CHB,INPUT);				  // VERTICAL
	
	//Joystick 2
	pinMode(CHF,INPUT_PULLUP);
	buttons[19].interval(5);
	buttons[19].attach(CHF); // CENTER
	pinMode(CHE,INPUT);				  // HORIZONTAL
	pinMode(CHD,INPUT);				  // VERTICAL
	
	//Potentiometer 1
	pinMode(CHG,INPUT);				  //
	
	//Potentiometer 2
	pinMode(CHH,INPUT);               //
	
	//Potentiometer 3
	pinMode(CHI,INPUT);               //
}


/*
// Color definitions
#define	BLACK   0x0000
#define	BLUE    0x001F
#define	RED     0xF800
#define	GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define Black           0x0000      //   0,   0,   0 
#define Navy            0x000F      //   0,   0, 128 
#define DarkGreen       0x03E0      //  0, 128,   0  
#define DarkCyan        0x03EF      //   0, 128, 128 
#define Maroon          0x7800      // 128,   0,   0 
#define Purple          0x780F      // 128,   0, 128 
#define Olive           0x7BE0      // 128, 128,   0 
#define PaleGreen       0xF7DE      // 192, 192, 192 
#define LightGrey       0xF7BE      // 192, 192, 192 
#define DarkGrey        0x7BEF      // 128, 128, 128 
#define Blue            0x001F      //   0,   0, 255 
#define Green           0x07E0      //   0, 255,   0 
#define Cyan            0x07FF      //   0, 255, 255 
#define Red             0xF800      // 255,   0,   0 
#define Magenta         0xF81F      // 255,   0, 255 
#define Yellow          0xFFE0      // 255, 255,   0 
#define White           0xFFFF      // 255, 255, 255 
#define ORANGE          0xFD20      // 255, 165,   0 
#define GreenYellow     0xAFE5      // 173, 255,  47 
#define Pink            0xF81F
*/
