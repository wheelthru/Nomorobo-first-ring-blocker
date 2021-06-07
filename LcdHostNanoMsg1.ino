/* Quickie standalone Arduino to receive messages from
 * NMR first ring stopper and display on LCD.  Uses custom
 * chars for cursor/clear control
 * 1/6/20
 * LcdHostNanoMsg1: 
 * Added protocol for serial message from NMR controller 1/10/20  works
*/

#include <LiquidCrystal_PCF8574.h>
#include <Wire.h>

LiquidCrystal_PCF8574 lcd(0x27); // set the LCD address to 0x27 for a 16 chars and 2 line display

#define HEADER (0x55)
int rings,nmrblocks;

int show = -1;

void setup()
{
  int error;

  Serial.begin(1200);
  //Serial.println("LCD...");


  // See http://playground.arduino.cc/Main/I2cScanner how to test for a I2C device.
  Wire.begin();
  Wire.beginTransmission(0x27);
  error = Wire.endTransmission();

  if (error == 0) {
    Serial.println(": LCD found at 0x27.");
    show = 0;
    lcd.begin(16, 2); // initialize the lcd

  } else {
    Serial.println(": LCD not found.");
  } // if

} // setup()


void loop()
{
  char blank16[]="                ";
  int bits[]={0,0x10,0x8,0x4,0x2,0x1,0x0,0},i;
  lcd.createChar(1,bits); // backslash at char pos 1
  char hbchar[]={'|','/','-',char(1)};
  static char errcnt='0';

    lcd.setBacklight(255);
    lcd.home();
    lcd.clear();
    lcd.print("Nomorobo LCD");
    delay(1000);

  while(1){
    int getvalret=getval();
    if(getvalret==-1)errcnt++;
    if(getvalret==1){
      lcd.setCursor(0,0);
      lcd.print(blank16);
      lcd.setCursor(0,0);
      lcd.print("rings: ");
      lcd.print(rings);
      
      lcd.setCursor(0,1);
      lcd.print(blank16);
      lcd.setCursor(0,1);
      lcd.print("NMR blocks: ");
      lcd.print(nmrblocks);
      
      lcd.setCursor(15,0);
      lcd.print(errcnt);
      lcd.setCursor(15,1);
      lcd.print(hbchar[i++]);
      if(i>3)i=0;
    }// end if got msg
    
  }//end while

  
} // loop()



//--------- READ NMR MESSAGE ---------
// return: -1=err; 0=nothing available; 1=got data
int getval(){
  static byte state=0;
  byte val;

  while(Serial.available()){
    val=Serial.read(); 
    switch(state){
      case 0: if(val==0x55)state=1; // waiting for header
      break;
      case 1: rings=val;  //got rings
        state=2;
        break;
      case 2: rings+=val<<8; // got nmrblocks
        state=3;
        break;
      case 3: nmrblocks=val;  //got rings
        state=4;
        break;
      case 4: nmrblocks+=val>>8; // got nmrblocks
        state=5;
        break;
     case 5: if(val==(byte)((rings&0xff)+(rings<<8)+(nmrblocks&0xff)+(nmrblocks<<8))){ // check sum
        state=6;
        }else{state=99;}  //fail
        break;
      case 6: if(val==0xa){
        state=0;
        return 1;
      }else{state=99;}
      break;
      default :
        state=99;
    }//end switch
  }// end while
 
  if(state==99){
    state=0;
    return -1;
  }//end if 99
 return 0; 
}// end getval
