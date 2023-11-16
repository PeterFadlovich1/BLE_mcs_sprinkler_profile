/*************************************************************************************************/
/*!
 * @file    main.c
 * @brief   Maxim custom Bluetooth profile and service that advertises as "MCS" and accepts
connection requests.
*
*  Copyright (c) 2013-2019 Arm Ltd. All Rights Reserved.
*
*  Copyright (c) 2019 Packetcraft, Inc.
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*/
/*************************************************************************************************/

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

#include "sec_api.h"
#include "hci_handler.h"
#include "dm_handler.h"
#include "l2c_handler.h"
#include "att_handler.h"
#include "smp_handler.h"
#include "l2c_api.h"
#include "att_api.h"
#include "smp_api.h"
#include "app_api.h"
#include "hci_core.h"
#include "app_terminal.h"
#include "mxc_delay.h"
#include "mcs_api.h"
#include "tmr.h"
#include "adc.h"
#include "lp.h"
#include "nvic_table.h"
#include "pb.h"
#include "board.h"
#include "gpio.h"

#if defined(HCI_TR_EXACTLE) && (HCI_TR_EXACTLE == 1)
#include "ll_init_api.h"
#endif

#include "pal_bb.h"
#include "pal_cfg.h"

#include "mcs_app_api.h"
#include "app_ui.h"
#include "mcs_api.h"
#include "BLE_handlers.h"

#include "solenoid_fun.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! \brief UART TX buffer size */
#define PLATFORM_UART_TERMINAL_BUFFER_SIZE 2048U
#define DEFAULT_TX_POWER 0 /* dBm */

//clock initializations
#define BLE_TIMER MXC_TMR2
#define MANUAL_TIMER MXC_TMR4

#define TAKE_SAMPLE_TIMER MXC_TMR1
#define SAMPLE_PERIOD_TIMER MXC_TMR4
#define PWM_TIMER MXC_TMR3
//solenoid timer on #0

#define MOISTURE_READ_1 MXC_ADC_CH_0
#define RAIN_READ_1 MXC_ADC_CH_1
#define RAIN_READ_2 MXC_ADC_CH_2
#define RAIN_VOLTAGE_THRESHOLD 0x1ff

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! \brief  Pool runtime configuration. */
static wsfBufPoolDesc_t mainPoolDesc[] = { { 16, 8 }, { 32, 4 }, { 192, 8 }, { 256, 8 } };

#if defined(HCI_TR_EXACTLE) && (HCI_TR_EXACTLE == 1)
static LlRtCfg_t mainLlRtCfg;
#endif

//BLE global variable initializations 
int manualOff = 0;
int manualOn = 0;
int manualTime = 0;
int rootDepth = 0;
int scheduleTimeArray[8] = { 0, 0, 0, 0, 0, 0, 0, 0};
int rainOn = 0;
int capOn = 0;
int solenoidState = 0;

//Data Storage Globals
int rainSensorData[5000];
int capSensorData[5000];

int batteryLow, scheduledTimeOn, raining, highMoisture, lowMoisture, scheduledTimeOff;
int moistureAvg, moistureCount, rainPeaks, rainCount;

/**************************************************************************************************
  Functions
**************************************************************************************************/

/*! \brief  Stack initialization for app. */
extern void StackInitMcsApp(void);

/*************************************************************************************************/
/*!
 *  \brief  Initialize WSF.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void mainWsfInit(void)
{
#if defined(HCI_TR_EXACTLE) && (HCI_TR_EXACTLE == 1)
    /* +12 for message headroom, + 2 event header, +255 maximum parameter length. */
    const uint16_t maxRptBufSize = 12 + 2 + 255;

    /* +12 for message headroom, +4 for header. */
    const uint16_t aclBufSize = 12 + mainLlRtCfg.maxAclLen + 4 + BB_DATA_PDU_TAILROOM;

    /* Adjust buffer allocation based on platform configuration. */
    mainPoolDesc[2].len = maxRptBufSize;
    mainPoolDesc[2].num = mainLlRtCfg.maxAdvReports;
    mainPoolDesc[3].len = aclBufSize;
    mainPoolDesc[3].num = mainLlRtCfg.numTxBufs + mainLlRtCfg.numRxBufs;
#endif

    const uint8_t numPools = sizeof(mainPoolDesc) / sizeof(mainPoolDesc[0]);

    uint16_t memUsed;
    WsfCsEnter();
    memUsed = WsfBufInit(numPools, mainPoolDesc);
    WsfHeapAlloc(memUsed);
    WsfCsExit();

    WsfOsInit();
    WsfTimerInit();
#if (WSF_TOKEN_ENABLED == TRUE) || (WSF_TRACE_ENABLED == TRUE)
    WsfTraceRegisterHandler(WsfBufIoWrite);
    WsfTraceEnable(TRUE);
#endif
}

/*************************************************************************************************/
/*!
*  \fn     setAdvTxPower
*
*  \brief  Set the default advertising TX power.
*
*  \return None.
*/
/*************************************************************************************************/
void setAdvTxPower(void)
{
    LlSetAdvTxPower(DEFAULT_TX_POWER);
}

//Added functions 

void manualTimerHandler(void)
{
    // Clear interrupt
    MXC_TMR_SetCount(MANUAL_TIMER,0);
    MXC_TMR_ClearFlags(MANUAL_TIMER);
    manualOff = 0;
    manualOn = 0;
    manualTime = 0;
    //manualInterruptInit();
    printf("Manual timer Handler Interrput Hit \n");
    fflush(stdout);

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

void manualInterruptInit(){
    //printf("Manual Interupt Init Start \n");
    //fflush(stdout);
    MXC_NVIC_SetVector(TMR4_IRQn, manualTimerHandler);
    NVIC_EnableIRQ(TMR4_IRQn);
    oneshotTimerInit(MANUAL_TIMER, 80,TMR_PRES_4096);//14400 = 30min //80 for 10s
    //printf("Manual Interupt Init End \n");
    //fflush(stdout);
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


//ADC sensor sampling functions ******************************************************************************
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

int adc_done, adcval;
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

#if 0
void rainTimerHandler(){
    /*code here will be based of off characterization*/
    raining = rainPeaks > 3 ? TRUE : FALSE;
    MXC_TMR_ClearFlags(SAMPLE_PERIOD_TIMER);
    //MXC_TMR_Stop(TAKE_SAMPLE_TIMER);
    //MXC_TMR_ClearFlags(TAKE_SAMPLE_TIMER);
    MXC_TMR_Shutdown(TAKE_SAMPLE_TIMER);
    printf("Rain one shot expired: %d\n", rainPeaks);
    fflush(stdout);
    return;
}

void moistureTimerHandler(){
    /*code here will be based off of characterization and rootdepth*/
    moistureAvg1 = moistureAvg1/moistureCount;
    moistureAvg2 = moistureAvg2/moistureCount;
    highMoisture = moistureAvg1 < 0x0ff;
    lowMoisture = moistureAvg2 > 0x2ff;
    MXC_TMR_Stop(SAMPLE_PERIOD_TIMER);

    printf("Cap one shot expired: %d\n", moistureAvg1);
    fflush(stdout);
    return;
}
#endif
void moistureStartMeasurement(){
    MXC_TMR_ClearFlags(TAKE_SAMPLE_TIMER);
    if(moistureCount%2 == 0){
        MXC_GPIO_OutSet(MXC_GPIO0,  MXC_GPIO_PIN_21);
        float adcval = MXC_ADC_StartConversion(MOISTURE_READ_1)*(1.22/1024);
        printf("ADC moisture 1 reading %f: \n", adcval);
        //MXC_GPIO_OutClr(MXC_GPIO0,  MXC_GPIO_PIN_21);
    }
    else{
        //MXC_GPIO_OutSet(MXC_GPIO0, MXC_GPIO_PIN_22);
        float adcval = MXC_ADC_StartConversion(MOISTURE_READ_1)*(1.22/1024);
        printf("ADC moisture 2 reading %f: \n", adcval);
        //MXC_GPIO_OutClr(MXC_GPIO0,  MXC_GPIO_PIN_22);
    }
    moistureCount++;
    moistureAvg+=adcval;
}


void rainStartMeasurement(){
    MXC_TMR_ClearFlags(TAKE_SAMPLE_TIMER);
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
    //MXC_NVIC_SetVector(TMR2_IRQn, moistureTimerHandler);
    //NVIC_EnableIRQ(ADC_IRQn);
    //MXC_NVIC_SetVector(TMR4_IRQn, rainTimerHandler);
    //MXC_NVIC_SetVector(TMR1_IRQn, rainStartMeasurement);
    //NVIC_EnableIRQ(TMR4_IRQn);
    NVIC_EnableIRQ(TMR1_IRQn);
    //oneshotTimerInit(SAMPLE_PERIOD_TIMER, 1875, TMR_PRES_1024); //~60s
    PWMTimerInit(PWM_TIMER,20,10,TMR_PRES_8); //~1ms period at 50% duty cycle
    //continuousTimerInit(TAKE_SAMPLE_TIMER, 2000, TMR_PRES_4096, FALSE); //~1s
    //MXC_TMR_Stop(TAKE_SAMPLE_TIMER);
    //MXC_TMR_SetCount(TAKE_SAMPLE_TIMER,0);
    printf("Sensor System Initialized \n");
    fflush(stdout);
}

void startMoistureSystem(){
    //MXC_NVIC_SetVector(TMR4_IRQn, moistureTimerHandler);

    MXC_NVIC_SetVector(TMR1_IRQn, moistureStartMeasurement);
    MXC_TMR_Start(PWM_TIMER);
    //MXC_TMR_Start(TAKE_SAMPLE_TIMER);
    //MXC_TMR_Start(SAMPLE_PERIOD_TIMER);
    //MXC_TMR_Start(SAMPLE_PERIOD_TIMER);

    continuousTimerInit(TAKE_SAMPLE_TIMER, 125, TMR_PRES_256, FALSE); //~1s Start of continuous 
    printf("Start Moisture");
    fflush(stdout);

}

void startRainSystem(){  
    printf("Start Rain \n");
    fflush(stdout);

    //MXC_NVIC_SetVector(TMR4_IRQn, rainTimerHandler);
    MXC_NVIC_SetVector(TMR1_IRQn, rainStartMeasurement);


    //MXC_TMR_Start(SAMPLE_PERIOD_TIMER);

    continuousTimerInit(TAKE_SAMPLE_TIMER, 125, TMR_PRES_256, FALSE); //~1s Start of continuous 

}

/*************************************************************************************************/
/*!
*  \fn     main
*
*  \brief  Entry point for demo software.
*
*  \param  None.
*
*  \return None.
*/
/*************************************************************************************************/
int main(void)
{
#if defined(HCI_TR_EXACTLE) && (HCI_TR_EXACTLE == 1)
    /* Configurations must be persistent. */
    static BbRtCfg_t mainBbRtCfg;

    PalBbLoadCfg((PalBbCfg_t *)&mainBbRtCfg);
    LlGetDefaultRunTimeCfg(&mainLlRtCfg);
#if (BT_VER >= LL_VER_BT_CORE_SPEC_5_0)
    /* Set 5.0 requirements. */
    mainLlRtCfg.btVer = LL_VER_BT_CORE_SPEC_5_0;
#endif
    PalCfgLoadData(PAL_CFG_ID_LL_PARAM, &mainLlRtCfg.maxAdvSets, sizeof(LlRtCfg_t) - 9);
#if (BT_VER >= LL_VER_BT_CORE_SPEC_5_0)
    PalCfgLoadData(PAL_CFG_ID_BLE_PHY, &mainLlRtCfg.phy2mSup, 4);
#endif

    /* Set the 32k sleep clock accuracy into one of the following bins, default is 20
      HCI_CLOCK_500PPM
      HCI_CLOCK_250PPM
      HCI_CLOCK_150PPM
      HCI_CLOCK_100PPM
      HCI_CLOCK_75PPM
      HCI_CLOCK_50PPM
      HCI_CLOCK_30PPM
      HCI_CLOCK_20PPM
    */
    mainBbRtCfg.clkPpm = 20;

    /* Set the default connection power level */
    mainLlRtCfg.defTxPwrLvl = DEFAULT_TX_POWER;
#endif

    uint32_t memUsed;
    WsfCsEnter();
    memUsed = WsfBufIoUartInit(WsfHeapGetFreeStartAddress(), PLATFORM_UART_TERMINAL_BUFFER_SIZE);
    WsfHeapAlloc(memUsed);
    WsfCsExit();

    mainWsfInit();
    AppTerminalInit();

#if defined(HCI_TR_EXACTLE) && (HCI_TR_EXACTLE == 1)
    WsfCsEnter();
    LlInitRtCfg_t llCfg = { .pBbRtCfg = &mainBbRtCfg,
                            .wlSizeCfg = 4,
                            .rlSizeCfg = 4,
                            .plSizeCfg = 4,
                            .pLlRtCfg = &mainLlRtCfg,
                            .pFreeMem = WsfHeapGetFreeStartAddress(),
                            .freeMemAvail = WsfHeapCountAvailable() };

    memUsed = LlInit(&llCfg);
    WsfHeapAlloc(memUsed);
    WsfCsExit();

    bdAddr_t bdAddr;
    PalCfgLoadData(PAL_CFG_ID_BD_ADDR, bdAddr, sizeof(bdAddr_t));
    LlSetBdAddr((uint8_t *)&bdAddr);
#endif

    StackInitMcsApp();
    McsAppStart();

    bluetoothInterruptInit();

    NVIC_SetPriority(TMR2_IRQn,0); //BLE highest priority
    NVIC_SetPriority(TMR4_IRQn,1);

    GPIOINIT();

    //manualInterruptInit();

    solenoidInit();

#if 0
    while(1){
        while(solenoidState == 0){




        }
        McsSetFeatures(1);
        printf("Solenoid Open\n");
        fflush(stdout);

        while(solenoidState == 1){

        }
        McsSetFeatures(0);
        printf("Solenoid Closed\n");
        fflush(stdout);

    }
#endif 

    while(1){
        if(rainOn == 1){
            initMoistureRainSystem();
            printf("after init\n");
            fflush(stdout);
            startRainSystem();
            printf("Start Rain If Statement \n");
            fflush(stdout);
            while(rainOn == 1){

            }

            ///rainOn = 0;
        }

 
        if(capOn == 1){
            initMoistureRainSystem();
            startMoistureSystem();

            while(capOn == 1){

            }

            //capOn = 0;
        }
    }
#if 0
    while(1){
        if(rainCount > 1){
            openSolenoid();
            printf("Open Solenoid\n");
            fflush(stdout);
        }
    }
    return 0;
#endif
}



