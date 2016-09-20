// C64MultiKernal.c
// Originally coded by BWACK in mikroC
// Modified and ported to MPLAB by discoHR

// Multikernal switcher for the C64 breadbin/longboard
// a 2332 ROM to 27C256 ROM adapter with four kernals

// This microcontroller program flips through the four kernals at the press of
// restore key, in other words its a 'switchless' design.
//
// Hold the restore key to enter the select mode. You will notice a fast flash
// on the LED. Release the restore key when the flashing stops and press it
// shortly as many times as you want to switch to the other kernals.
// The LED colour will indicate which kernal is selected.
// The C64 will reset to selected kernal shortly after you stop pressing the
// restore key.
//
// The Reset button (if present) behaves similar to the restore key but
// unlike the restore key, the reset button is always in select mode.
//
// There are two LED colour themes but due to hardware limitations, they don't
// differ much. You can hold the restore key during power-on to toggle between
// the two themes.
// Theme #1: red, green, blue, cyan
// Theme #2: red, lime, magenta, cyan

// MCU: PIC12F629 8 pin PDIP
// use any pic-programmer and load the .hex file

// CONFIG
#pragma config FOSC = INTRCIO   // Oscillator Selection bits (INTOSC oscillator: I/O function on GP4/OSC2/CLKOUT pin, I/O function on GP5/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = OFF      // Power-Up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = OFF      // GP3/MCLR pin function select (GP3/MCLR pin function is digital I/O, MCLR internally tied to VDD)
#pragma config BOREN = OFF      // Brown-out Detect Enable bit (BOD disabled)
#pragma config CP = OFF         // Code Protection bit (Program Memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#define _XTAL_FREQ  4000000

#include <xc.h>
#include <pic12f629.h>

// inputs
#define RESTORE_N   GPIObits.GP3

// outputs
#define RED_LED     GPIObits.GP2
#define A13         GPIObits.GP4
#define A14         GPIObits.GP5

// input/outputs
#define INTRST_N    GPIObits.GP1     // open-collector

// EEPROM addresses used for configuration persistance
#define EEPROM_ADDR_KERNAL          0
#define EEPROM_ADDR_RED_INVERTED    2

#define byte unsigned char

enum state_t {
    IDLE, SELECT
} state = IDLE;

byte buttonTimer, old_button;
byte kernalIndex, oldKernalIndex;
byte ignoreReset;
byte redInverted;

void SetKernal(byte index) {
    GPIO = (GPIO & 0x0f) | (index << 4);
    __delay_ms(20);
}

void SaveKernal(byte index) {
    // don't write unless necessary
    if (index != oldKernalIndex) {
        EEPROM_WRITE(EEPROM_ADDR_KERNAL, index);
        oldKernalIndex = index;
    }
}

void DoReset(void) {
    INTRST_N = 0;
    TRISIObits.TRISIO1 = 0; // pull INTRES_N low
    RED_LED ^= 1;
    __delay_ms(50);
    RED_LED ^= 1;
    __delay_ms(200);
    TRISIObits.TRISIO1 = 1; // release INTRES_N
    __delay_ms(250); // possible fix for double-reset cartridges
}

void SetLED(void) {
    RED_LED = ((0 != kernalIndex) && redInverted) ^ ((buttonTimer & 2) ? 0 : 1);
}

void Init(void) {
    byte i;

    OPTION_REG = 0;
    WPUbits.WPU1 = 1;
    CMCON = 0x07; // digital IO
    TRISIO = 0b00001011;
    INTRST_N = 1;
    RESTORE_N = 1;
    RED_LED = 0;
    kernalIndex = oldKernalIndex = EEPROM_READ(EEPROM_ADDR_KERNAL) & 3;
    SetKernal(kernalIndex);
    redInverted = EEPROM_READ(EEPROM_ADDR_RED_INVERTED) & 1;
    DoReset();

    for (i = 0; i < 10; i++) {
        RED_LED = 0;
        __delay_ms(50);
        RED_LED = 1;
        __delay_ms(50);
    }

    __delay_ms(250); // ignore reset during power up
}

void main() {
    Init();

    // change the colour theme if user is holding the restore key on power-on
    if (!RESTORE_N) {
        redInverted ^= 1;
        EEPROM_WRITE(EEPROM_ADDR_RED_INVERTED, redInverted);
        // wait until restore key is released
        while (!RESTORE_N) {
            __delay_ms(100);
        }
    }

    // main loop
    for (;;) {
        SetLED();
        switch (state) {
            case IDLE:
                if (!RESTORE_N) {
                    ++buttonTimer;
                } else {
                    buttonTimer = 0;
                }
                if (buttonTimer > 15 || !INTRST_N) {
                    // either the restore key was long-pressed or
                    // the reset button was short-pressed
                    state = SELECT;
                    old_button = buttonTimer = 0;
                    // ignore reset in the select state if it was the reset button
                    ignoreReset = !INTRST_N;
                    RED_LED ^= 1;
                    __delay_ms(50);
                    RED_LED ^= 1;
                    __delay_ms(50);
                }
                break;

            case SELECT:
                if (!old_button && RESTORE_N && INTRST_N) {
                    old_button = 1;
                } else if (old_button && (!RESTORE_N || !INTRST_N)) {
                    old_button = ignoreReset = 0; // it's ok to reset after this
                    ++kernalIndex;
                    kernalIndex &= 3;
                    SetKernal(kernalIndex);
                } else if (RESTORE_N && INTRST_N) {
                    // both buttons released
                    ++buttonTimer;
                    if (buttonTimer > 30) {
                        // do the actual reset if needed and go back to idle state
                        old_button = 1;
                        if (!ignoreReset) {
                            DoReset();
                        }
                        SaveKernal(kernalIndex);
                        buttonTimer = 0;
                        state = IDLE;
                    }
                } else {
                    // at least one button is pressed
                    buttonTimer = 0;
                }
                break;

            default:
                // something is wrong, flash like crazy
                A13 = A14 = 0; // pull A13 and A14 low, makes red clearly visible
                RED_LED ^= 1;
                break;
        }
        __delay_ms(50);
    }
}