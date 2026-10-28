/*
 * smoke-receiver.h
 *
 * Project header for the smoke/temperature receiver.
 * Replaces TI's RF_Toggle_LED_Demo.h (include-only, restrictive license).
 *
 * Copyright (c) 2022 Pengyu Li
 * SPDX-License-Identifier: MIT
 */

#ifndef SMOKE_RECEIVER_H
#define SMOKE_RECEIVER_H

#include "smoke_config.h"
#include "cc430x513x.h"
#include "RF1A.h"
#include "HAL_PMM.h"

void Transmit(unsigned char *buffer, unsigned char length);
void ReceiveOn(void);
void ReceiveOff(void);
void InitRadio(void);

#endif /* SMOKE_RECEIVER_H */
