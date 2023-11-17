#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "tmr.h"
#include "adc.h"


void oneshotTimerInit(mxc_tmr_regs_t *timer, uint32_t ticks, mxc_tmr_pres_t prescalar);

void continuousTimerInit(mxc_tmr_regs_t *timer, uint32_t ticks, mxc_tmr_pres_t prescalar, int enable);

void PWMTimerInit(mxc_tmr_regs_t *timer, uint32_t ticks, uint32_t dutyTicks, mxc_tmr_pres_t prescalar);

void bluetoothInterruptHandler();

void bluetoothInterruptInit();

void initADC(mxc_adc_monitor_t monitor, mxc_adc_chsel_t chan, uint16_t hithresh);

void moistureStartMeasurement();

void rainStartMeasurement();

void initMoistureRainSystem();

void startMoistureSystem();

void startRainSystem();

int chargeBattery();

void offLoop();

void onLoop();


#endif