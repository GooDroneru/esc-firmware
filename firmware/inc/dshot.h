/*
 * dshot.h
 *
 *  Created on: Apr. 22, 2020
 *      Author: Alka
 */

#pragma once

#include "main.h"

void computeDshotDMA(void);
void make_dshot_package(uint16_t com_time);

extern void playInputTune(void);
extern void playInputTune2(void);
extern void playBeaconTune3(void);
extern void saveEEpromSettings(void);

extern volatile char dshot_telemetry;
extern volatile char armed;
extern char dir_reversed;
extern char buffer_divider;
extern uint8_t last_dshot_command;
extern volatile uint32_t commutation_interval;

// DShot frequency lock & commutation sync
extern uint32_t clock_scale;
extern volatile uint8_t dshot_sync_pending;
extern volatile uint8_t dshot_calibration_failed;

// int e_com_time;
