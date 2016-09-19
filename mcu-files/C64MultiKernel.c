// C64MultiKernel.c Rev 1.1-disco (2016-09-10)
// coded by BWACK in mikroC
// -disco variant with instant select-mode on reset-button press

// Multikernel switcher for the C64 breadbin/longboard
// a 2332 ROM to 27C256 ROM adapter with four kernels
// The microcontroller program flips through the four kernels at the press of
// restore key, in other words its a 'switchless' design.

// MCU: PIC12F629 8 pin PDIP
// EasyPIC5 development board
// use any pic-programmer and load the .hex file

// Inputs:
#define RESTORE_N GPIO.B3
// Outputs:
#define RED_LED   GPIO.B2
#define INTRST_N  GPIO.B1 // open-collector
// Addresses on GPIO B4 and B5

// finite state machine
#define IDLE_STATE 0
//#define RESET_STATE 1
#define SELECT_STATE 2


char STATE=IDLE_STATE;
char buttontimer=0, old_button;
char kernalno=0;
char ignorereset=0;

void setkernal(char _kernal) {
  GPIO.B4=0;
  GPIO.B5=0;
  GPIO|=kernalno<<4;
  EEPROM_Write(0x00,kernalno);
}

void intres(void) {
  INTRST_N=0;
  TRISIO.B1=0; // pull INTRES_N low
  RED_LED=~RED_LED;
  delay_ms(50);
  RED_LED=~RED_LED;
  delay_ms(200); // was 500
  TRISIO.B1=1; // release INTRES_N
  delay_ms(500); // potential fix for double-reset cartridges
}

void setLED(void) {
  if(!GPIO.B4 && !GPIO.B5) RED_LED= 1;
  else RED_LED=0;
}

void init(void) {
  char _i;
  
  OPTION_REG=0;
  WPU.WPU1=1;
  CMCON=0x07; // digital IO
//ANSEL=0; // only defined for pic12f675
  TRISIO=0b00001011;
  INTRST_N=1;
  RESTORE_N=1;
  RED_LED=0;
  kernalno=EEPROM_READ(0x00);
  kernalno&=3; // in case of EEPROM garbage
  setkernal(kernalno);
  intres();

  for(_i=0; _i<10; _i++) {
    RED_LED=1;
    delay_ms(50);
    RED_LED=0;
    delay_ms(50);
  }
  
  delay_ms(250); // ignore reset during power up
}

void main() {
  init();
  while(1) {
    setLED();
    if(STATE==IDLE_STATE) {
      if(!RESTORE_N)
        buttontimer++;
      else
        buttontimer=0;
      delay_ms(100);
      if (buttontimer>15 || !INTRST_N) {
         // either the restore key was long-pressed or the reset button was short-pressed
         STATE=SELECT_STATE;
         old_button=buttontimer=0;
         ignorereset=!INTRST_N; // ignore reset in the SELECT_STATE if it was the reset button
         RED_LED=~RED_LED;
         delay_ms(50);
         RED_LED=~RED_LED;
         delay_ms(50);
      }
    } else if(STATE==SELECT_STATE) {
      if(!old_button && RESTORE_N && INTRST_N ) {
        old_button=1;
        delay_ms(20);
      }

      if(old_button && (!RESTORE_N || !INTRST_N) ) {
        old_button=ignorereset=0; // it's ok to reset after this
        kernalno++;
        kernalno&=0x03;
        setkernal(kernalno);
        delay_ms(20);
      }

      if (RESTORE_N && INTRST_N) {
        // both buttons are released
        buttontimer++;
        delay_ms(50);
      } else {
        // at least one button is pressed
        buttontimer=0;
      }

      if (buttontimer > 30) {
        // do the actual reset if needed and go back to idle state
        STATE=IDLE_STATE;
        old_button=1;
        if (!ignorereset) {
          intres();
        }
        buttontimer=0;
      }
    }
  }
}