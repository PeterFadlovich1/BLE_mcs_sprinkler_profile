#define CAP_CYCLES 5
#define ADC_SOIL_1 3
#define ADC_SOIL_2 4
#define ADC_RAIN 6
#define RAIN_VOLTAGE_THRESHOLD 0x1ff
#define RAIN_TIME 120
#define SOLENOID_ON 1
#define SOLENOID_OFF 0
#define MANUAL_TIMER MXC_TMR0
#define BLE_TIMER MXC_TMR1
//#include "ADC_lib.h"
#include "tmr.h"
#include "mxc_delay.h"
#include "adc.h"
#include "stdio.h"
int batteryLow, manualOn, scheduledTimeOn, manualOff, raining, highMoisture, lowMoisture, scheduledTimeOff, manualTime, adc_done, adc_val;

void adc_complete_cb(void *req, int adcRead)
{
    adc_val = adcRead;
    adc_done = 1;
    return;
}
void ADC_IRQHandler(void)
{
    MXC_ADC_Handler();
    printf("%d\n", adc_done);
    fflush(stdout);
    adc_done = 0;
}

int onLoop(){
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

int offLoop(){
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

int readSoil(int *soil1, int *soil2){
    int cycles;
    int avg1, avg2 = 0;
    PWM_init();
    for(cycles = 0; cycles < CAP_CYCLES; cycles++){
        avg1 += ADC_conversion(ADC_SOIL_1);
        MXC_Sleep(500000);
    }
    *soil1 = avg1/CAP_CYCLES;
    for(cycles = 0; cycles < CAP_CYCLES; cycles++){
        avg2 += ADC_conversion(ADC_SOIL_2);
        MXC_Sleep(500000);
    }
    *soil2 = avg2/CAP_CYCLES;
    return 0;
}

int getRainStatus(int avg, int outliers){
    return 0;
}

int readRain(){
    int avg, count, outliers, reading = 0;
    MXC_RTC_Start();
    while(MXC_RTC_GetSeconds() < RAIN_TIME){
        reading = ADC_conversion(ADC_RAIN);
        if (reading < RAIN_VOLTAGE_THRESHOLD){
            outliers++;
        }
        avg+=reading;
        count++;
    }
    avg = avg/count;
    return getRainStatus(avg, outliers);
}

int setSolenoid(int desired_state){
    return 0;
}

int chargeBattery(){
    setSolenoid(SOLENOID_ON);
    MXC_Delay(60000000);
    setSolenoid(SOLENOID_OFF);
    return 0;
}
void manualTimerHandler(void)
{
    // Clear interrupt
    MXC_TMR_ClearFlags(MANUAL_TIMER);
    manualOff = 0;
    manualOn = 0;
    manualTime = 0;

}

void rainCompleteTimerHandler(void){

}

void moistureCompleteTimerHandler(void){

}
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
    tmr.clock = MXC_TMR_32K_CLK;
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

            OneshotTimer();

    printf("Oneshot timer started.\n\n");

    //MXC_TMR_Start(OST_TIMER);
}

void manualInterruptInit(){
    MXC_NVIC_SetVector(TMR0_IRQn, manualTimerHandler);
    NVIC_EnableIRQ(TMR0_IRQn);
    oneshotTimerInit(MANUAL_TIMER, 14400,TMR_PRES_4096);
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
    tmr.clock = MXC_TMR_8M_CLK;
    tmr.cmp_cnt =ticks; //SystemCoreClock*(1/interval_time);
    tmr.pol = 0;

    if (enable == TRUE && MXC_TMR_Init(timer, &tmr, true) != E_NO_ERROR) {
        printf("Failed Continuous timer Initialization.\n");
        return;
    }
}

void bluetoothInterruptHandler(){
    MXC_TMR_ClearFlags(BLE_TIMER);
    WsfTimerSleepUpdate();
    wsfOsDispatcher();
    if (!WsfOsActive())
    {

        WsfTimerSleep();

    }
}

void bluetoothInterruptInit(){
    MXC_NVIC_SetVector(TMR1_IRQn, bluetoothInterruptHandler);
    NVIC_EnableIRQ(TMR1_IRQn);
    continuousTimerInit(BLE_TIMER, 49,TMR_PRES_4096, TRUE); //temp value of 25ms
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
}