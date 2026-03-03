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

extern char dshot_telemetry;
extern char armed;
extern char dir_reversed;
extern char buffer_divider;
extern uint8_t last_dshot_command;
extern uint32_t commutation_interval;

// int e_com_time;


