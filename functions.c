#include "tmr.h"
#include <stdbool.h>
#include <stdio.h>
#include "adc.h"
#include "solenoid_fun.h"
#include "BLE_handlers.h"
#include "mxc_delay.h"
#include "nvic_table.h"
#include "lp.h"

#include <string.h>
#include "wsf_types.h"
#include "wsf_trace.h"
#include "wsf_bufio.h"
#include "wsf_msg.h"
#include "wsf_assert.h"
#include "wsf_buf.h"
#include "wsf_heap.h"
#include "wsf_cs.h"
#include "wsf_timer.h"
#include "wsf_os.h"







#define BLE_TIMER MXC_TMR2
#define PWM_TIMER MXC_TMR3
#define TAKE_SAMPLE_TIMER MXC_TMR1


#define MOISTURE_READ_1 MXC_ADC_CH_0
#define RAIN_READ_1 MXC_ADC_CH_1
#define RAIN_READ_2 MXC_ADC_CH_2
#define RAIN_VOLTAGE_THRESHOLD 0x1ff



int moistureCount;
int moistureSum;
int moistureAvg;
int capSensorData[5000];

int rainCount;
int rainPeaks;
int rainSensorData[5000];


int manualOff = 0;
int manualOn = 0;
int manualTime = 0;
int rootDepth = 0;
int scheduleTimeArray[8] = { 0, 0, 0, 0, 0, 0, 0, 0};
int rainOn = 0;
int capOn = 0;

int scheduledTimeOn;
int scheduledTimeOff;
int batteryLow;
int lowMoisture;
int highMoisture;
int raining;


void oneshotTimerInit(mxc_tmr_regs_t *timer, uint32_t ticks, mxc_tmr_pres_t prescalar)
{
    // Declare variables
    mxc_tmr_cfg_t tmr;
    /*
    Steps for configuring a timer for PWM mode:
    1. Disable the timer
    2. Set the prescale value
    3  Configure the timer for continuous mode
    4. Set polarity, timer parameters
    5. Enable Timer
    */

    MXC_TMR_Shutdown(timer);

    tmr.pres = prescalar;
    tmr.mode = TMR_MODE_ONESHOT;
    tmr.bitMode = TMR_BIT_MODE_32;
    tmr.clock = MXC_TMR_32K_CLK; //was 32k switched to 8k for timer 5 usage as a one shot
    tmr.cmp_cnt = ticks; //SystemCoreClock*(1/interval_time);
    tmr.pol = 0;

    if (MXC_TMR_Init(timer, &tmr, true) != E_NO_ERROR) {
        printf("Failed Continuous timer Initialization.\n");
        return;
    }
    
    MXC_TMR_EnableInt(timer);

    // Clear Wakeup status
    MXC_LP_ClearWakeStatus();
    // Enable wkup source in Poower seq register
    MXC_LP_EnableTimerWakeup(timer);
    // Enable Timer wake-up source
    MXC_TMR_EnableWakeup(timer, &tmr);

    //OneshotTimer();

    //printf("Oneshot timer started.\n\n");

    //MXC_TMR_Start(timer);
    
}



void continuousTimerInit(mxc_tmr_regs_t *timer, uint32_t ticks, mxc_tmr_pres_t prescalar, int enable)
{
    // Declare variables
    mxc_tmr_cfg_t tmr;

    /*
    Steps for configuring a timer for PWM mode:
    1. Disable the timer
    2. Set the prescale value
    3  Configure the timer for continuous mode
    4. Set polarity, timer parameters
    5. Enable Timer
    */
    MXC_TMR_Shutdown(timer);

    tmr.pres = prescalar;
    tmr.mode = TMR_MODE_CONTINUOUS;
    tmr.bitMode = TMR_BIT_MODE_16B;
    tmr.clock = MXC_TMR_32K_CLK;
    tmr.cmp_cnt =ticks; //SystemCoreClock*(1/interval_time);
    tmr.pol = 0;



    if (MXC_TMR_Init(timer, &tmr, true) != E_NO_ERROR) {
        printf("Failed Continuous timer Initialization.\n");
        return;
    }
    if (enable == FALSE){
        
        MXC_TMR_Stop(timer);
        //MXC_TMR_SetCount(timer, 0);
        //MXC_TMR_ClearFlags(timer);
        printf("Stop/Reset timer \n");
        fflush(stdout);
    }

    return;
#if 0
    if (enable == TRUE && MXC_TMR_Init(timer, &tmr, true) != E_NO_ERROR) {
        printf("Failed Continuous timer Initialization.\n");
        int flag = MXC_TMR_Init(timer, &tmr, true);
        printf("Error Flag %d \n", flag);
        return;
    }
#endif
}

void PWMTimerInit(mxc_tmr_regs_t *timer, uint32_t ticks, uint32_t dutyTicks, mxc_tmr_pres_t prescalar)
{
    // Declare variables
    mxc_tmr_cfg_t tmr; // to configure timer
    //unsigned int periodTicks = MXC_TMR_GetPeriod(PWM_TIMER, PWM_CLOCK_SOURCE, 16, FREQ);
    //unsigned int dutyTicks = periodTicks * DUTY_CYCLE / 100;

    /*
    Steps for configuring a timer for PWM mode:
    1. Disable the timer
    2. Set the pre-scale value
    3. Set polarity, PWM parameters
    4. Configure the timer for PWM mode
    5. Enable Timer
    */

    MXC_TMR_Shutdown(PWM_TIMER);

    tmr.pres = prescalar;
    tmr.mode = TMR_MODE_PWM;
    tmr.bitMode = TMR_BIT_MODE_32;
    tmr.clock = MXC_TMR_8M_CLK;
    tmr.cmp_cnt = ticks;
    tmr.pol = 1;

    if (MXC_TMR_Init(timer, &tmr, true) != E_NO_ERROR) {
        printf("Failed PWM timer Initialization.\n");
        return;
    }

    if (MXC_TMR_SetPWM(timer, dutyTicks) != E_NO_ERROR) {
        printf("Failed TMR_PWMConfig.\n");
        return;
    }

    //MXC_TMR_Start(PWM_TIMER);

    //printf("PWM started.\n\n");
}


void bluetoothInterruptHandler(){
    MXC_TMR_ClearFlags(BLE_TIMER);
    WsfTimerSleepUpdate();
    wsfOsDispatcher();
    if (!WsfOsActive())
    {

        WsfTimerSleep();

    }
    //printf("BLE interupt \n");
    //fflush(stdout);
}

void bluetoothInterruptInit(){
    MXC_NVIC_SetVector(TMR2_IRQn, bluetoothInterruptHandler);
    NVIC_EnableIRQ(TMR2_IRQn);
    continuousTimerInit(BLE_TIMER, 25,TMR_PRES_32, TRUE); //temp value of 25ms ~ 25 
}



void initADC(mxc_adc_monitor_t monitor, mxc_adc_chsel_t chan, uint16_t hithresh){
    if (MXC_ADC_Init() != E_NO_ERROR) {
        printf("Error Bad Parameter\n");

    }

    /* Set up LIMIT0 to monitor high and low trip points */
    MXC_ADC_SetMonitorChannel(monitor, chan);
    MXC_ADC_SetMonitorHighThreshold(monitor, hithresh);
    MXC_ADC_SetMonitorLowThreshold(monitor, 0);
    MXC_ADC_EnableMonitor(monitor);

    //NVIC_EnableIRQ(ADC_IRQn);
}




void moistureStartMeasurement(){
    MXC_TMR_ClearFlags(TAKE_SAMPLE_TIMER);
    float adcval;
    if(moistureCount%2 == 0){
        MXC_GPIO_OutSet(MXC_GPIO0,  MXC_GPIO_PIN_21);
        MXC_Delay(500000);
        adcval = MXC_ADC_StartConversion(MOISTURE_READ_1)*(1.22/1024);
        printf("ADC moisture 1 reading %f: \n", adcval);
        
        MXC_GPIO_OutClr(MXC_GPIO0,  MXC_GPIO_PIN_21);
    }
    else{
        MXC_GPIO_OutSet(MXC_GPIO0, MXC_GPIO_PIN_22);
        MXC_Delay(500000);
        adcval = MXC_ADC_StartConversion(MOISTURE_READ_1)*(1.22/1024);
        printf("ADC moisture 2 reading %f: \n", adcval);
        
        MXC_GPIO_OutClr(MXC_GPIO0,  MXC_GPIO_PIN_22);
    }
    moistureCount++;
    moistureSum+=adcval;
    moistureAvg = moistureSum/moistureCount;
    capSensorData[moistureCount-1] = moistureAvg;
}


void rainStartMeasurement(){
    MXC_TMR_ClearFlags(TAKE_SAMPLE_TIMER);
    int adcval;
    printf("Rain Start Conversion \n");
    fflush(stdout);
    if(rainCount%2 == 0){
        adcval = MXC_ADC_StartConversion(RAIN_READ_1);
        printf("ADC reading %u: \n", adcval);
    }
    else{
        adcval = MXC_ADC_StartConversion(RAIN_READ_2);
        printf("ADC reading %u: \n", adcval);
    }
    rainSensorData[rainCount] = adcval;
    rainCount++;
    printf("Rain Count %u: \n", rainCount);
    if (adcval < RAIN_VOLTAGE_THRESHOLD){
        rainPeaks++;
    }
}


void initMoistureRainSystem(){
    initADC(MXC_ADC_MONITOR_0, MOISTURE_READ_1, 0x1ff); //One ADC channel for moisture, select with GPIO to amps. load switch as well?
    initADC(MXC_ADC_MONITOR_1, RAIN_READ_1, 0x1ff);
    initADC(MXC_ADC_MONITOR_2, RAIN_READ_2, 0x1ff); // Two ADC channels for rain, 

    NVIC_EnableIRQ(TMR1_IRQn);
    //oneshotTimerInitTMR0(SAMPLE_PERIOD_TIMER, 300, TMR_PRES_1024); //~60s
    PWMTimerInit(PWM_TIMER,30,15,TMR_PRES_8); //~1ms period at 50% duty cycle 20/10 for 50k ~ 37.5k

    printf("Sensor System Initialized \n");
    fflush(stdout);
}

void startMoistureSystem(){
    MXC_GPIO_OutSet(MXC_GPIO2,  MXC_GPIO_PIN_6);

    MXC_NVIC_SetVector(TMR1_IRQn, moistureStartMeasurement);
    MXC_TMR_Start(PWM_TIMER);

    continuousTimerInit(TAKE_SAMPLE_TIMER, 125, TMR_PRES_256, FALSE); //~1s Start of continuous 
    printf("Start Moisture");
    fflush(stdout);
    //capOn = 0;

}

void startRainSystem(){  
    MXC_GPIO_OutSet(MXC_GPIO2,  MXC_GPIO_PIN_6);

    printf("Start Rain \n");
    fflush(stdout);

    MXC_NVIC_SetVector(TMR1_IRQn, rainStartMeasurement);

    continuousTimerInit(TAKE_SAMPLE_TIMER, 125, TMR_PRES_256, FALSE); //~1s Start of continuous
    //rainOn = 0; 

}

int chargeBattery(){
    printf("Battery Charging");
    fflush(stdout);
    openSolenoid();
    MXC_Delay(60000000);
    closeSolenoid();
    return 0;
}
//TRUE;
//FALSE;

int offLoop(){
    while(1){
        if(batteryLow){
            chargeBattery();
        }
        else if(manualOn){
            break;
        }
        else if(scheduledTimeOn && !manualOff && !raining && !highMoisture){
            break;
        }
        else if(lowMoisture && !manualOff && !raining){
            break;
        }
    }
    return 0;
}

int onLoop(){
    while(1){
        if(manualOn){
            continue;
        }
        else if(manualOff || raining || highMoisture || scheduledTimeOff){
            break;
        }
    }
    return 0;
}
