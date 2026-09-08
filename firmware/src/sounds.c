/*
 * sounds.c
 *
 *  Created on: May 13, 2020
 *      Author: Alka
 */

#include "sounds.h"
#include "common.h"
#include "eeprom.h"
#include "functions.h"
#include "peripherals.h"
#include "phaseouts.h"
#include "targets.h"

uint8_t beep_volume;

void pause(uint16_t ms)
{
    SET_DUTY_CYCLE_ALL(0);
	delayMillis(ms);
    SET_DUTY_CYCLE_ALL(beep_volume); // volume of the beep, (duty cycle) don't go
                                     // above 25 out of 2000
}

void setVolume(uint8_t volume)
{
    if (volume > 11) {
		volume = 11;
	}
	beep_volume = volume * 2;           // volume variable from 0 - 11 equates to CCR value of 0-22
}

void setCaptureCompare()
{
    SET_DUTY_CYCLE_ALL(beep_volume); // volume of the beep, (duty cycle) don't go
                                     // above 25 out of 2000
}

void playBJNote(uint16_t freq, uint16_t bduration)
{ // hz and ms
	uint16_t timerOne_reload = TIM1_AUTORELOAD;

    SET_PRESCALER_PWM(10);
    timerOne_reload = CPU_FREQUENCY_MHZ*100000 / freq;

    SET_AUTO_RELOAD_PWM(timerOne_reload);
    SET_DUTY_CYCLE_ALL(beep_volume * timerOne_reload / TIM1_AUTORELOAD); // volume of the beep, (duty cycle) don't
                                                                         // go above 25 out of 2000
	delayMillis(bduration);
}

uint16_t getBlueJayNoteFrequency(uint8_t bjarrayfreq)
{
    return 10000000 / (bjarrayfreq * 247 + 4000);
}

void playBlueJayTune()
{
	uint8_t full_time_count = 0;
	uint16_t duration;
    uint16_t frequency;
	comStep(3);
    // read_flash_bin(blueJayTuneBuffer , EEPROM_START_ADD + 48 , 128);
    for (int i = 52; i < 176; i += 2) {
        RELOAD_WATCHDOG_COUNTER();
		signaltimeout = 0;

        if (eepromBuffer.buffer[i] == 255) {
			full_time_count++;

        } else {
            if (eepromBuffer.buffer[i + 1] == 0) {
                duration = full_time_count * 254 + eepromBuffer.buffer[i];
                SET_DUTY_CYCLE_ALL(0);
				delayMillis(duration);
                RELOAD_WATCHDOG_COUNTER();
            }
            else
            {
                frequency = getBlueJayNoteFrequency(eepromBuffer.buffer[i + 1]);
                duration = ((full_time_count * 254 + eepromBuffer.buffer[i]) * (100000 / frequency)) / 100;
			    playBJNote(frequency, duration);
                RELOAD_WATCHDOG_COUNTER();
			}
			full_time_count = 0;
		}
	}
	allOff();                // turn all channels low again
    SET_PRESCALER_PWM(0); // set prescaler back to 0.
    SET_AUTO_RELOAD_PWM(TIMER1_MAX_ARR);
	signaltimeout = 0;
    RELOAD_WATCHDOG_COUNTER();
}

void playStartupTune()
{
	__disable_irq();

    //uint8_t value = *(uint8_t*)(EEPROM_START_ADD + 48);
//    if (value != 0xFF) {
//		playBlueJayTune();
//    } else {
    SET_AUTO_RELOAD_PWM(TIM1_AUTORELOAD);
	setCaptureCompare();
	comStep(3);       // activate a pwm channel
    SET_PRESCALER_PWM(55); // frequency of beep
    delayMillis(200); // duration of beep
    RELOAD_WATCHDOG_COUNTER();
    //signaltimeout = 0;
    comStep(5);
    SET_PRESCALER_PWM(40); // next beep is higher frequency
    delayMillis(200);
    RELOAD_WATCHDOG_COUNTER();
    //signaltimeout = 0;
	comStep(6);
    SET_PRESCALER_PWM(25); // higher again..
    delayMillis(200);
    RELOAD_WATCHDOG_COUNTER();
	allOff();                // turn all channels low again
    SET_PRESCALER_PWM(0); // set prescaler back to 0.
	signaltimeout = 0;
	//}

    SET_AUTO_RELOAD_PWM(TIMER1_MAX_ARR);
	__enable_irq();
}

void playBrushedStartupTune()
{
	__disable_irq();
    SET_AUTO_RELOAD_PWM(TIM1_AUTORELOAD);
	setCaptureCompare();
	comStep(1);       // activate a pwm channel
    SET_PRESCALER_PWM(40); // frequency of beep
    delayMillis(300);         // duration of beep
    RELOAD_WATCHDOG_COUNTER();
	comStep(2);       // activate a pwm channel
    SET_PRESCALER_PWM(30); // frequency of beep
    delayMillis(300);         // duration of beep
    RELOAD_WATCHDOG_COUNTER();
	comStep(3);       // activate a pwm channel
    SET_PRESCALER_PWM(25); // frequency of beep
    delayMillis(300);         // duration of beep
    RELOAD_WATCHDOG_COUNTER();
    comStep(4);
    SET_PRESCALER_PWM(20); // higher again..
	delayMillis(300);
    RELOAD_WATCHDOG_COUNTER();
	allOff();                // turn all channels low again
    SET_PRESCALER_PWM(0); // set prescaler back to 0.
	signaltimeout = 0;
    SET_AUTO_RELOAD_PWM(TIMER1_MAX_ARR);
	__enable_irq();
}

void playDuskingTune()
{
	setCaptureCompare();
    SET_AUTO_RELOAD_PWM(TIM1_AUTORELOAD);
	comStep(2);       // activate a pwm channel
    SET_PRESCALER_PWM(60); // frequency of beep
    delayMillis(200);         // duration of beep
    RELOAD_WATCHDOG_COUNTER();
    SET_PRESCALER_PWM(55); // next beep is higher frequency
	delayMillis(150);
    RELOAD_WATCHDOG_COUNTER();
    SET_PRESCALER_PWM(50); // higher again..
	delayMillis(150);
    RELOAD_WATCHDOG_COUNTER();
    SET_PRESCALER_PWM(45); // frequency of beep
    delayMillis(100);         // duration of beep
    RELOAD_WATCHDOG_COUNTER();
    SET_PRESCALER_PWM(50); // next beep is higher frequency
	delayMillis(100);
    RELOAD_WATCHDOG_COUNTER();
    SET_PRESCALER_PWM(55); // higher again..
	delayMillis(100);
    RELOAD_WATCHDOG_COUNTER();
    SET_PRESCALER_PWM(25); // higher again..
	delayMillis(200);
    RELOAD_WATCHDOG_COUNTER();
    SET_PRESCALER_PWM(55); // higher again..
	delayMillis(150);
    RELOAD_WATCHDOG_COUNTER();
	allOff();                // turn all channels low again
    SET_PRESCALER_PWM(0); // set prescaler back to 0.
    SET_AUTO_RELOAD_PWM(TIMER1_MAX_ARR);
}

void playInputTune2()
{
    SET_AUTO_RELOAD_PWM(TIM1_AUTORELOAD);
	__disable_irq();
    RELOAD_WATCHDOG_COUNTER();
    SET_PRESCALER_PWM(60);
	setCaptureCompare();
	comStep(1);
    delayMillis(75);
    RELOAD_WATCHDOG_COUNTER();
    SET_PRESCALER_PWM(80);
    delayMillis(75);
    RELOAD_WATCHDOG_COUNTER();
    SET_PRESCALER_PWM(90);
    RELOAD_WATCHDOG_COUNTER();
    delayMillis(75);
    RELOAD_WATCHDOG_COUNTER();
	allOff();
    SET_PRESCALER_PWM(0);
	signaltimeout = 0;
    SET_AUTO_RELOAD_PWM(TIMER1_MAX_ARR);
	__enable_irq();
}

void playInputTune()
{
	__disable_irq();
    SET_AUTO_RELOAD_PWM(TIM1_AUTORELOAD);
    RELOAD_WATCHDOG_COUNTER();
    SET_PRESCALER_PWM(80);
	setCaptureCompare();
	comStep(3);
    signaltimeout = 0;
    delayMillis(100);
    RELOAD_WATCHDOG_COUNTER();
    SET_PRESCALER_PWM(70);
    delayMillis(100);
    RELOAD_WATCHDOG_COUNTER();
    //signaltimeout = 0;
    SET_PRESCALER_PWM(40);
    delayMillis(100);
    RELOAD_WATCHDOG_COUNTER();
	allOff();
    SET_PRESCALER_PWM(0);
	signaltimeout = 0;
    SET_AUTO_RELOAD_PWM(TIMER1_MAX_ARR);
	__enable_irq();
}

void playDefaultTone()
{
    SET_AUTO_RELOAD_PWM(TIM1_AUTORELOAD);
    SET_PRESCALER_PWM(50);
	setCaptureCompare();
	comStep(2);
	delayMillis(150);
    RELOAD_WATCHDOG_COUNTER();
    RELOAD_WATCHDOG_COUNTER();
    SET_PRESCALER_PWM(30);
	delayMillis(150);
    RELOAD_WATCHDOG_COUNTER();
	allOff();
    SET_PRESCALER_PWM(0);
	signaltimeout = 0;
    SET_AUTO_RELOAD_PWM(TIMER1_MAX_ARR);
}

void playChangedTone()
{
    SET_AUTO_RELOAD_PWM(TIM1_AUTORELOAD);
    SET_PRESCALER_PWM(40);
	setCaptureCompare();
	comStep(2);
	delayMillis(150);
    RELOAD_WATCHDOG_COUNTER();
    RELOAD_WATCHDOG_COUNTER();
    SET_PRESCALER_PWM(80);
	delayMillis(150);
    RELOAD_WATCHDOG_COUNTER();
	allOff();
    SET_PRESCALER_PWM(0);
	signaltimeout = 0;
    SET_AUTO_RELOAD_PWM(TIMER1_MAX_ARR);
}

void playBeaconTune3()
{
    SET_AUTO_RELOAD_PWM(TIM1_AUTORELOAD);
	__disable_irq();
	setCaptureCompare();
    for (int i = 119; i > 0; i = i - 2)
    {
        RELOAD_WATCHDOG_COUNTER();
        comStep(i / 20);
        SET_PRESCALER_PWM(10 + (i / 2));
		delayMillis(10);
        RELOAD_WATCHDOG_COUNTER();
	}
	allOff();
    SET_PRESCALER_PWM(0);
	signaltimeout = 0;
    SET_AUTO_RELOAD_PWM(TIMER1_MAX_ARR);
	__enable_irq();
}

// ---------------------------------------------------------------------------
// Morse error codes
// Digits are 5 elements each, played on one phase at a fixed pitch.
// ---------------------------------------------------------------------------

#define MORSE_UNIT_MS 60

/* bit4..bit0 = 5 elements in time order, 0 = dot, 1 = dash */
static const uint8_t morse_digit_pattern[10] = {
    0b11111, // 0  -----
    0b01111, // 1  .----
    0b00111, // 2  ..---
    0b00011, // 3  ...--
    0b00001, // 4  ....-
    0b00000, // 5  .....
    0b10000, // 6  -....
    0b11000, // 7  --...
    0b11100, // 8  ---..
    0b11110, // 9  ----.
};

static void morsePlayDigit(uint8_t digit)
{
	uint8_t pattern = morse_digit_pattern[digit % 10];
	for (int i = 4; i >= 0; i--) {
		RELOAD_WATCHDOG_COUNTER();
		SET_DUTY_CYCLE_ALL(beep_volume);
		delayMillis(((pattern >> i) & 1) ? MORSE_UNIT_MS * 3 : MORSE_UNIT_MS);
		SET_DUTY_CYCLE_ALL(0);
		delayMillis(MORSE_UNIT_MS); // gap between elements
	}
}

/* Plays a two digit error code (e.g. 10 -> "----- .----").
 * Structured exactly like the other tunes: IRQs masked for the duration
 * (keeps commutation sources from switching the phases mid-tone), watchdog
 * reloaded per element. NOTE: signaltimeout is intentionally NOT touched -
 * resetting it here would stop the no-signal reboot from ever firing while
 * an error code is being played repeatedly. */
void playMorseErrorCode(uint8_t code)
{
	__disable_irq();
	SET_AUTO_RELOAD_PWM(TIM1_AUTORELOAD);
	comStep(3);
	SET_PRESCALER_PWM(50);
	setCaptureCompare();
	morsePlayDigit(code / 10);
	delayMillis(MORSE_UNIT_MS * 3); // gap between digits
	morsePlayDigit(code % 10);
	allOff();
	SET_PRESCALER_PWM(0);
	SET_AUTO_RELOAD_PWM(TIMER1_MAX_ARR);
	__enable_irq();
}
