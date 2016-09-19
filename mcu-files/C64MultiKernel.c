// C64MultiKernal.c
// Coded by BWACK in mikroC
// Modified by discoHR to

// Multikernal switcher for the C64 breadbin/longboard
// a 2332 ROM to 27C256 ROM adapter with four kernals

// This microcontroller program flips through the four kernals at the press of
// restore key, in other words its a 'switchless' design.
// Hold the restore key to enter the select mode. You will notice a fast flash
// on the LED. Release the restore key and press it shortly to switch to the
// next kernal. The LED colour will indicate which kernal is selected.
// The C64 will reset to selected kernal shortly after you stop pressing the
// restore key.
// The Reset button (if present) behaves similar to the restore key but
// unlike the restore key, the reset button is always in select mode.

// MCU: PIC12F629 8 pin PDIP
// use any pic-programmer and load the .hex file

// inputs
#define RESTORE_N   GPIO.B3

// outputs
#define RED_LED     GPIO.B2
#define A13         GPIO.B4
#define A14         GPIO.B5

// input/outputs
#define INTRST_N    GPIO.B1 // open-collector

#define byte unsigned char

enum StateType {IDLE, SELECT} state = IDLE;

byte buttonTimer, old_button;
byte kernalIndex;
byte ignoreReset;

void SetKernal(char kernalIndex) {
    A13 = A14 = 0;
    GPIO |= kernalIndex << 4;
    EEPROM_Write(0x00, kernalIndex);
    Delay_ms(20);
}

void DoReset(void) {
    INTRST_N = 0;
    TRISIO.B1 = 0; // pull INTRES_N low
    RED_LED ^= 1;
    Delay_ms(50);
    RED_LED ^= 1;
    Delay_ms(200);
    TRISIO.B1 = 1; // release INTRES_N
    Delay_ms(250); // possible fix for double-reset cartridges
}

void SetLED(void) {
    RED_LED = (buttonTimer & 2) ? 0 : 1;
}

void Init(void) {
    byte i;

    OPTION_REG = 0;
    WPU.WPU1 = 1;
    CMCON = 0x07; // digital IO
    TRISIO = 0b00001011;
    INTRST_N = 1;
    RESTORE_N = 1;
    RED_LED = 0;
    kernalIndex = EEPROM_Read(0);
    kernalIndex &= 3; // in case of EEPROM garbage
    SetKernal(kernalIndex);
    DoReset();

    for (i = 0; i < 10; i++) {
        RED_LED = 0;
        Delay_ms(50);
        RED_LED = 1;
        Delay_ms(50);
    }

    Delay_ms(250); // ignore reset during power up
}

void main() {
  Init();

  for (;;) {
    SetLED();
    switch (state) {
        case IDLE:
            if (!RESTORE_N) {
                buttonTimer++;
            } else {
                buttonTimer=0;
            }
            if (buttonTimer > 15 || !INTRST_N) {
                // either the restore key was long-pressed or
                // the reset button was short-pressed
                state = SELECT;
                old_button = buttonTimer = 0;
                // ignore reset in the select state if it was the reset button
                ignoreReset = !INTRST_N;
                RED_LED ^= 1;
                Delay_ms(50);
                RED_LED ^= 1;
                Delay_ms(50);
            }
            break;
          
        case SELECT:
            if (!old_button && RESTORE_N && INTRST_N) {
                old_button=1;
            } else if (old_button && (!RESTORE_N || !INTRST_N)) {
                old_button = ignoreReset = 0; // it's ok to reset after this
                ++kernalIndex;
                kernalIndex &= 3;
                SetKernal(kernalIndex);
            } else if (RESTORE_N && INTRST_N) {
                // both buttons are released
                ++buttonTimer;
                if (buttonTimer > 30) {
                    // do the actual reset if needed and go back to idle state
                    old_button = 1;
                    if (!ignoreReset) {
                        DoReset();
                    }
                    buttonTimer=0;
                    state = IDLE;
                }
            } else {
                // at least one button is pressed
                buttonTimer=0;
            }
            break;
          
        default:
            RED_LED ^= 1; // something is wrong, flash like crazy
            break;
      }
      Delay_ms(50);
   }
}