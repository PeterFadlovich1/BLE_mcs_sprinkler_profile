#include "tmr.h"
#include <stdbool.h>
#include <stdio.h>
#include "adc.h"
#include "solenoid_fun.h"
#include "BLE_handlers.h"
#include "mxc_delay.h"
#include "nvic_table.h"
#include "lp.h"
#include "rtc.h"

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
#include "svc_mcs.h"






#define BLE_TIMER MXC_TMR2
#define PWM_TIMER MXC_TMR3
#define TAKE_SAMPLE_TIMER MXC_TMR1


#define MOISTURE_READ_1 MXC_ADC_CH_0
#define RAIN_READ_1 MXC_ADC_CH_0 //1
#define RAIN_READ_2 MXC_ADC_CH_2


#define RAIN_VOLTAGE_THRESHOLD 350
#define RAIN_PEAK_THRESHOLD 3

#define LOW_MOISTURE_THRESHOLD 754 
#define HIGH_MOISTURE_THRESHOLD 511



int moistureCount;
int moistureSum;
int moistureAvg;
int capStorageCount;
uint8_t capSensorData[64];

int rainCount;
uint8_t rainPeaks;
int rainStorageCount;
uint8_t rainSensorData[64];


int manualOff = 0;
int manualOn = 0;
int manualTime = 0;
int rootDepth = 0;
int scheduleTimeArray[6] = { 0, 0, 0, 0, 0, 0};
int currentRealTimeSec = 0;
int nextTimeIndex = 0;
int rainOn = 0;
int capOn = 0;

int scheduledTimeOn = 0;
int scheduledTimeOff = 0;
int batteryLow;
int lowMoisture;
int highMoisture;
int raining;

int stateDataCount = 0;
uint8_t stateData[64];


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



//Set as dependent on root depth instead of alternating between sampling the two. 
//Make sure the delays within samples are fine 
void moistureStartMeasurement(){
    MXC_TMR_ClearFlags(TAKE_SAMPLE_TIMER);
    int adcval;
    if(rootDepth == 0){
        if(moistureCount%2 == 0){
            MXC_GPIO_OutSet(MXC_GPIO0,  MXC_GPIO_PIN_21);
            MXC_Delay(500000);
            adcval = MXC_ADC_StartConversion(MOISTURE_READ_1);//*(1.22/1024);
            printf("ADC moisture 1 reading %f: \n", adcval*(1.22/1024));
            
            MXC_GPIO_OutClr(MXC_GPIO0,  MXC_GPIO_PIN_21);
        }
        else{
            MXC_GPIO_OutSet(MXC_GPIO0, MXC_GPIO_PIN_22);
            MXC_Delay(500000);
            adcval = MXC_ADC_StartConversion(MOISTURE_READ_1);//*(1.22/1024);
            printf("ADC moisture 2 reading %f: \n", adcval*(1.22/1024));
            
            MXC_GPIO_OutClr(MXC_GPIO0,  MXC_GPIO_PIN_22);
        }
    }
    else if(rootDepth < 4){
            MXC_GPIO_OutSet(MXC_GPIO0,  MXC_GPIO_PIN_21);
            MXC_Delay(500000);
            adcval = MXC_ADC_StartConversion(MOISTURE_READ_1);//*(1.22/1024);
            printf("ADC moisture 1 reading %f: \n", adcval*(1.22/1024));
            
            MXC_GPIO_OutClr(MXC_GPIO0,  MXC_GPIO_PIN_21);
    }
    else{
            MXC_GPIO_OutSet(MXC_GPIO0, MXC_GPIO_PIN_22);
            MXC_Delay(500000);
            adcval = MXC_ADC_StartConversion(MOISTURE_READ_1);//*(1.22/1024);
            printf("ADC moisture 2 reading %f: \n", adcval*(1.22/1024));
            
            MXC_GPIO_OutClr(MXC_GPIO0,  MXC_GPIO_PIN_22);
    }



    moistureCount++;
    moistureSum+=adcval;
    moistureAvg = moistureSum/moistureCount;

    //printf("ADC moisture type cast to 8 bit %u: \n", (uint8_t) adcval);
    //fflush(stdout);
    //capSensorData[moistureCount-1] = (uint8_t)  (adcval >> 2);


    //Make sure we are losing the 2 LSB with the typecast and not the 2 MSB
    //Avg thresholds for low and high moisture?
    //Stop sampling when variable flipped?
}

void capEnd(){
    capSensorData[capStorageCount] = (uint8_t)  (moistureAvg / 4);
    capStorageCount++;

    if (moistureAvg > HIGH_MOISTURE_THRESHOLD){
        highMoisture = 1;
        lowMoisture = 0;
    }
    else if (moistureAvg < LOW_MOISTURE_THRESHOLD){
        highMoisture = 0;
        lowMoisture = 1;
    }
    else{
        highMoisture = 0;
        lowMoisture = 0;
    }

    moistureCount = 0;
    moistureSum = 0;
    moistureAvg = 0;
}

void rainStartMeasurement(){
    MXC_TMR_ClearFlags(TAKE_SAMPLE_TIMER);
    int adcval;
    printf("Rain Start Conversion \n");
    fflush(stdout);
    adcval = MXC_ADC_StartConversion(RAIN_READ_1);//*(1.22/1024);
    printf("ADC reading %f: \n", adcval*(1.22/1024));
    #if 0
    if(rainCount%2 == 0){
        adcval = MXC_ADC_StartConversion(RAIN_READ_1);//*(1.22/1024);
        printf("ADC reading %f: \n", adcval*(1.22/1024));
    }
    else{
        adcval = MXC_ADC_StartConversion(RAIN_READ_2);//*(1.22/1024);
        printf("ADC reading %f: \n", adcval*(1.22/1024));
    }
    #endif
    //rainSensorData[rainCount] = (uint8_t) (adcval >> 2);
    rainCount++;
    printf("Rain Count %u: \n", rainCount);
    if (adcval < RAIN_VOLTAGE_THRESHOLD){
        rainPeaks++;
        printf("Peak count %u: \n",rainPeaks);
        fflush(stdout);
    }
    //# of rainPeaks needed to flip raining variable?
    //Stop sampling when raining is detected?

}


void rainEnd(){
    int rainPercent = 0;
    rainPercent = rainPeaks/rainCount;

    rainSensorData[rainStorageCount] = rainPeaks;
    rainStorageCount++;

    if(rainPeaks > RAIN_PEAK_THRESHOLD){
        raining = 1;
        printf("Raining Triggered, latest peak count: %u", rainSensorData[rainStorageCount-1]);
        fflush(stdout);
        //AttsSetAttr(MCS_DATA_HDL, 64, rainSensorData);

    }
    else{
        raining = 0;
    }
    rainCount = 0;
    rainPeaks = 0;
}



void initMoistureRainSystem(){
    initADC(MXC_ADC_MONITOR_0, MOISTURE_READ_1, 0x1ff); //One ADC channel for moisture, select with GPIO to amps. load switch as well?
    initADC(MXC_ADC_MONITOR_1, RAIN_READ_1, 0x1ff);
    initADC(MXC_ADC_MONITOR_2, RAIN_READ_2, 0x1ff); // Two ADC channels for rain, 

    NVIC_EnableIRQ(TMR1_IRQn);
    //oneshotTimerInitTMR0(SAMPLE_PERIOD_TIMER, 300, TMR_PRES_1024); //~60s
    PWMTimerInit(PWM_TIMER,30,15,TMR_PRES_8); //~1ms period at 50% duty cycle 20/10 for 50k ~ 37.5k 30/15

    printf("Sensor System Initialized \n");
    fflush(stdout);
}

void startMoistureSystem(){
    MXC_GPIO_OutSet(MXC_GPIO2,  MXC_GPIO_PIN_6);

    MXC_NVIC_SetVector(TMR1_IRQn, moistureStartMeasurement);
    MXC_TMR_Start(PWM_TIMER);

    continuousTimerInit(TAKE_SAMPLE_TIMER, 125, TMR_PRES_256, FALSE); //~1s Start of continuous ~ 125
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

//assumes times are entered sequentially 
void scheduleRTCHandler(){
    uint32_t time = 0;
    MXC_RTC_ClearFlags(MXC_RTC_GetFlags());
    printf("RTC Handler Hit \n");
    MXC_RTC_GetSeconds(&time);
    printf("second count %u \n", time);
    fflush(stdout);

    if(nextTimeIndex==3){
        nextTimeIndex = 0;
    }
    else{
        nextTimeIndex++;
    }

    if(nextTimeIndex%2 != 0){
        scheduledTimeOn = 1;
        scheduledTimeOff = 0;
    }
    else{
        scheduledTimeOn = 0;
        scheduledTimeOff = 1;
    }

    printf("next time index: %u \n", nextTimeIndex);
    printf("In schedule?: %u \n", scheduledTimeOn);
    fflush(stdout);



    while (MXC_RTC_DisableInt(MXC_F_RTC_CTRL_TOD_ALARM_IE) == E_BUSY) {}
        // Disable interrupt while re-arming RTC alarm

    if (MXC_RTC_SetTimeofdayAlarm(scheduleTimeArray[nextTimeIndex]) !=E_NO_ERROR) { // Reset TOD alarm for TIME_OF_DAY_SEC in the future
        /* Handle Error */
    }

    while (MXC_RTC_EnableInt(MXC_F_RTC_CTRL_TOD_ALARM_IE) == E_BUSY) {}
    // Re-enable TOD alarm interrupt

 }


//Assumed a profile has been loaded and RTC is started 
void storeStateChange(){
    uint32_t time = 0;
    MXC_RTC_GetSeconds(&time);
    uint8_t hour, minute;

    //hour = (time/3600)%24;
    //minute = (time%3600)/60;
    hour = time/5;
    minute = time%5;

    printf("Hour: %u \n Minute: %u", hour, minute);
    fflush(stdout);
    stateData[stateDataCount] = hour;
    stateDataCount++;
    stateData[stateDataCount] = minute;
    stateDataCount++;


    AttsSetAttr(MCS_STATE_DATA_HDL, 64, stateData);

    return;
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
    openSolenoid();
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
    closeSolenoid();
    return 0;
}
