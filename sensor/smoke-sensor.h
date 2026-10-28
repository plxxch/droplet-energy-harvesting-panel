/*
 * smoke-sensor.h
 *
 * Project header for the smoke/temperature sensor node.
 * Replaces TI's RF_Toggle_LED_Demo.h (include-only, restrictive license).
 *
 * Copyright (c) 2022 Pengyu Li
 * SPDX-License-Identifier: MIT
 */

#ifndef SMOKE_SENSOR_H
#define SMOKE_SENSOR_H

#include "smoke_config.h"
#include "cc430x513x.h"
#include "RF1A.h"

void Transmit(unsigned char *buffer, unsigned char length);
void ReceiveOn(void);
void ReceiveOff(void);
void InitRadio(void);

#endif /* SMOKE_SENSOR_H */
