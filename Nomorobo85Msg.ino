/* Nomorobo first ring stopper  started 12/30/19
 *  Production version, hosted on Tiny85.
 *  Operates a relay to disconnect all house phones
 *  on first ring.  If NMR blocks a call, we'll never hear
 *  anything.  If it lets the call thru, we need to look
 *  right away, just like in the old days.
 *  
 *  This is designed for our VOIP phones, with 2.0 sec
 *  of 20Hz 90VAC ring on top of 48VDC, 4 sec off.  We turn on
 *  a relay to open the line to the rest of the house phones as
 *  soon as we detect a ring - a few ms.  We watch each half cycle
 *  of the ring (full wave rectified thru a bridge) and detect 
 *  end of ring when we're not retriggered in 25ms or so.
 *  
 *  After first ring is over the NC relay is released, reconnecting
 *  all the house phones.  While that lets caller ID thru, since
 *  phones didn't hear a ring, they're not listening for caller ID
 *  until the next ring.
 *  
 *  Voltage divider (33K/1.5K) is set for 2.56V reference.
 *  
 *  The analog in line can be in one of 3 states:
 *  - 48V, on hook, idle
 *  - ringing: A/D 1023 ~15ms, < THRESH ~10ms
 *  - ~10V, off hook
 *  
 *  There's a ~4ms glitch of ~60V (enough to look like a ring) going from 
 *  off hook to on hook.  We have to protect against that being interpreted
 *  as a ring.  It would look like a NMR one ringer, and get a false count.
 *  
 *  The state machine has 3 states:
 *  - WAITING: 99% of time here.  48VDC, waiting for first ring
 *  - FIRSTRING: line voltage > THRESH within last 35ms, relay open
 *  - HOLDOFF: relay closed, won't retrigger and reopen relay; blue LED on;
 *    exit this state with continuous 5 sec idle.
 * ------------------------------------------------
 * Tiny85 version also counts rings and blocks and sends those stats out via
 * 1200B software serial to an opto isolated input on a 485 net node.  Because
 * the opto is an inverter, we use Software Serial's optional last param to invert.
 * Protocol is modeled after Basement Watchdog Big Dog voltage monitor:
 * - 0x55 header
 * - one byte # rings this message
 * - one byte # blocks this message
 * - one byte checksum
 * 
 * As an interim implementation, it sends ASCII text with some privately
 * defined controls to another Arduino that runs an LCD for display.  A few
 * lines of code change (in one function) and it will talk to a 485 node
 * thru the isolator.  Real Soon Now.
 * 
 * Nomorobo85Msg: changes from strings to formatted message 1/10/20 - works
 */

#include <jim.h>
#include <SoftwareSerial.h>


// hardware pin definitions
#define INPIN A2
#define RELAY 3
#define BLUELED 2
#define TXPIN 0
#define DONTCARE 1
#define HEADER (0x55)

// A/D min value if ringing 
#define THRESH 1020
// A/D max value if off hook
#define OFFHOOKVAL 400
// worst case time since last ring before declaring ring done, ms
#define RINGSTOPPED 35
// escape from HOLDOFF if idle continuously for this long
#define HOLDOFFTIME 5000

  SoftwareSerial myserial(DONTCARE,TXPIN,0);  // last param=1 to invert thru opto


void setup() {
  pinMode (BLUELED, OUTPUT);
  blink(BLUELED,20);
  delay(100);
  blink(BLUELED,20);
  pinMode(RELAY,OUTPUT);
  analogReference(INTERNAL2V56_NO_CAP);  // 2.56V on Tiny85
  myserial.begin(1200);
  //myserial.println("hello");

}// end setup


  enum l {idle,ring,offhook}line;
  int rings=0,nmrblocks=0;


//=============== MAIN LOOP =================
void loop() {
  long currmillis,lastring,lastspin,holdstart;
  int val,oldval=848;
  enum states {WAITING=0,FIRSTRING=1,HOLDOFF=2} state;
  bool morerings,doannounce; // true if ring during holdoff, if announce when done

  state=WAITING;
  
  while(1){
    currmillis=millis();
    // reduce noise with rolling average
    val=(analogRead(INPIN)+(oldval*5)+3)/6;
    oldval=val;
    // assign physical state of line
    line=idle;
    if(val>THRESH)line=ring;
    if(val<OFFHOOKVAL)line=offhook;
    
    //------ HANDLE DETECTED RING: VOLTAGE > THRESH ----
    if(line==ring){
      switch (state){
        case WAITING: // waiting for a ring - got one!
          digitalWrite(RELAY,HIGH); // block that ring!
          state=FIRSTRING;
          lastring=currmillis;    // need to know time since last ring pulse
          rings++;        // fact
          nmrblocks++;    // optimistic guess; fix later if wrong
          morerings=FALSE;  // don't count as NMR block if more rings
          break;
          
        case FIRSTRING: //actively in first ring
          lastring=currmillis;  // reset retrigger timer
          break;
          
        case HOLDOFF: // relay closed again
          morerings=TRUE;   // only matters if doannounce is true
          holdstart=currmillis; // reset timer
          break;
        default:
        ;
      }//end switch
     }//end if ringing; continue below with else not ringing


     //------ HANDLE VOLTAGE BELOW THRESH ------
     // could be idle, offhook, or a voltage dip during active ringing
     else { // idle OR offhook
      switch(state){
        case WAITING:   
          // Offline has a HV glitch when going onhook, so we just use HOLDOFF
          // so that glitch isn't perceived as a ring.
          if(line==offhook){
            state=HOLDOFF;
            doannounce=FALSE;
            holdstart=currmillis; //reset timer
            digitalWrite(BLUELED,HIGH);
          }//end if offhook
          // else boring 99% case
          break;
          
        case FIRSTRING:
          //Check for done ringing with timeout of > 1/2 positive cycle time
          if(currmillis-lastring > RINGSTOPPED){
            digitalWrite(RELAY,LOW);  // main reconnect
            state=HOLDOFF;  // don't detect ring until back to WAITING
            holdstart=currmillis; //reset timer
            doannounce=TRUE;  // only announce if we come in this way
            digitalWrite(BLUELED,HIGH); // visible indication of HOLDOFF state
            }//end if > RINGSTOPPED
            break;
            
        case HOLDOFF:
          // only way out is HOLDOFFTIME in idle state
          if(ring==offhook){holdstart=currmillis;}  // reset
          if(currmillis-holdstart > HOLDOFFTIME){
            state=WAITING;
            digitalWrite(BLUELED,LOW);
            if(doannounce){
              doannounce=FALSE;
              if(morerings)nmrblocks--;
              //announce(); //called in spinner
              }//end if doannounce
          }//end if > HOLDOFFTIME
            break; 
     }//end switch      
   }//end else voltage not > THRESH
      

    // SPINNER
    // was animate remote LCD
    // now, send data
    if(currmillis-lastspin > 2000){
     lastspin=currmillis+1;
     announce();
     //myserial.print('#');
   }//end lastspin
      
     
  }//end while
  
}// end loop()
//================================================



void blink(int pin,long dur){
  digitalWrite(pin,HIGH);
  delay(dur);
  digitalWrite(pin,LOW);  
}


// routine to annouce ring counts
void announce(){
// this is for messages
#if 1
  byte sum,b1,b2,b3,b4;
  b1=rings&0xff;
  b2=rings>>8;
  b3=nmrblocks&0xff;
  b4=nmrblocks>>8;
  myserial.write(HEADER);
  myserial.write(b1);
  myserial.write(b2);
  myserial.write(b3);
  myserial.write(b4);
  sum=b1+b2+b3+b4;  // sum is only a byte, so auto truncates
  myserial.write(sum);
  myserial.write(0xa);
#endif

// this is for remote LCD
#if 0
  myserial.print("$Rings: ");
  myserial.print(rings);
  myserial.print("&NMR blocks: ");
  myserial.print(nmrblocks);
#endif
  
// This one's for the local LCD
#if 0
    lcd.home();
    lcd.clear();
    lcd.print("Rings: ");
    lcd.print(rings);
    lcd.setCursor(0, 1);
    lcd.print("NMR blocks: ");
    lcd.print(nmrblocks);  
#endif

}// end announce
