#include <stdbool.h>
#include <stdio.h>
#include "app_ui.h"
#include "mcs_api.h"
#include "tmr.h"
#include "solenoid_fun.h"
#include "functions.h"
#include "rtc.h"
#include "mxc_delay.h"
#include "nvic_table.h"

#define MANUAL_TIMER MXC_TMR4
#define TAKE_SAMPLE_TIMER MXC_TMR1
#define PWM_TIMER MXC_TMR3


extern int manualOff;
extern int manualOn;
extern int manualTime;
extern int rootDepth;
extern int scheduleTimeArray[6];
extern int currentRealTimeSec;
extern int nextTimeIndex;
extern int rainOn;
extern int capOn;
extern int scheduledTimeOn;
extern int scheduledTimeOff;

extern uint8_t capSensorData[64];
extern uint8_t rainSensorData[64];


void manualOnOffHandler(uint16_t handle, uint8_t *pValue)
{
    //printf("sent value: %u \n", *pValue);
    //fflush(stdout);

    switch(*pValue){
        case 1: //Manual ON
        
            manualOn = 1;
            manualOff = 0;
            manualTime = 1;
            //MXC_TMR_Shutdown(MANUAL_TIMER);
            //MXC_TMR_Start(MANUAL_TIMER);


            //openSolenoid();

            printf("Manual On \n");
            fflush(stdout);

            break;
        case 2:  //Manual Off

            manualOn = 0;
            manualOff = 1;
            manualTime = 1;
            //MXC_TMR_Shutdown(MANUAL_TIMER);
            //MXC_TMR_Start(MANUAL_TIMER);

            //closeSolenoid();

            printf("Manual Off \n");
            fflush(stdout);

            break;
        case 3:  //Cancel Timer     app sends 3 when app side timer expires and resets micro timer
            manualOn = 0;
            manualOff = 0;
            manualTime = 0;
            //MXC_TMR_Stop(MANUAL_TIMER);
            //MXC_TMR_SetCount(MANUAL_TIMER,0);
            //MXC_TMR_ClearFlags(MANUAL_TIMER);

            printf("Cancel Timer \n");
            fflush(stdout);

            break;
    }
    //printf("manual Off = 1: %u", manualOff);
    //printf("\n");
    //fflush(stdout);

    AttsSetAttr(handle, 1, 0);

}

int timeConverter(uint8_t time){
    int hour = (time/4)*3600; 
    int minute = (time%4)*15*60; //add back in the *60
    return hour+minute;

}

void scheduleArrayHandler(uint16_t len, uint8_t *pValue)
{
    int i = 0;
    printf("callback hit");
    fflush(stdout);
    for (i = 0; i < len; i++){
        //printf("pValue test: %u \n \n", *(pValue + i));
        if(i<4){
            scheduleTimeArray[i] = timeConverter(*(pValue + i));
        }
        else{
            scheduleTimeArray[i] = *(pValue + i);
        }
        
        printf("Array test: %u \n", scheduleTimeArray[i]);
    }
    currentRealTimeSec = scheduleTimeArray[len-2]*3600+scheduleTimeArray[len-1]*60;//add back in the *60
    printf("real time: %u \n", currentRealTimeSec);
    fflush(stdout);

    //RTC initialization
    int j = 0;
    for (j = 0; j < len-2; j++){
        if(scheduleTimeArray[j]>currentRealTimeSec){
            nextTimeIndex = j;
            break;
        }
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

    if (MXC_RTC_Init(currentRealTimeSec, 0) != E_NO_ERROR) {
        printf("Failed RTC Initialization\n");
        printf("Example Failed\n");
        fflush(stdout);

        while (1) {}
    }

    if (MXC_RTC_Start() != E_NO_ERROR) {
        printf("Failed RTC_Start\n");
        printf("Example Failed\n");

        while (1) {}
    }


    
    MXC_NVIC_SetVector(RTC_IRQn, scheduleRTCHandler);

    NVIC_EnableIRQ(RTC_IRQn);



    if (MXC_RTC_SetTimeofdayAlarm(scheduleTimeArray[nextTimeIndex]) != E_NO_ERROR) {
            /* Handle Error */
    }

    while (MXC_RTC_EnableInt(MXC_F_RTC_CTRL_TOD_ALARM_IE) == E_BUSY) {}



}

void rootDepthHandler( uint8_t *pValue)
{
    rootDepth = *pValue;
    printf("root depth: %u", rootDepth);
    printf("\n");
    fflush(stdout);
}

void onSensorSet( uint8_t *pValue){

    switch(*pValue){
        case 1: //rain on
        //rainOn = 1;
        startRainSystem();
        //printf("rainOn: %u \n", rainOn);
        //fflush(stdout);
        break;

        case 2: //rain off
        //rainOn = 0;
        //printf("rainOff: %u \n", rainOn);
        //fflush(stdout);

        //ADD a check for if the sensor is currently sampling
        MXC_TMR_Shutdown(TAKE_SAMPLE_TIMER);
        rainEnd();
        MXC_GPIO_OutClr(MXC_GPIO2,  MXC_GPIO_PIN_6);
        break;

        case 3: //cap On
        //capOn = 1;
        startMoistureSystem();
        //printf("capOn: %u \n", capOn);
        //fflush(stdout);
        break;

        case 4: //cap Off
        //capOn = 0;
        //printf("capOff: %u \n", capOn);
        //fflush(stdout);
        MXC_TMR_Shutdown(TAKE_SAMPLE_TIMER);
        MXC_TMR_Stop(PWM_TIMER);
        capEnd();
        MXC_GPIO_OutClr(MXC_GPIO2,  MXC_GPIO_PIN_6);
        break;
    }
}




void loadData(uint8_t *pValue){
    //uint8_t testArray[512];
    switch(*pValue){
        case 1: //Rain Data
        printf("Case 1: %u", *pValue);
        printf("\n");
        fflush(stdout);
        
        AttsSetAttr(MCS_DATA_HDL, 64, rainSensorData);
        for(int x = 0; x<10;x++){
            printf("Rain Data %u \n", rainSensorData[x]);
            fflush(stdout);
        }

        break;

        case 2: //Cap Data

        AttsSetAttr(MCS_DATA_HDL, 64, capSensorData);
        
        break;
    }

}

//10 bit samples but 8 bit int transmission?

uint8_t testArray[64];
void requestData(){
    int i = 0;
    for(i = 0; i <64; i++){
        testArray[i] = 100;
    }

    AttsSetAttr(MCS_DATA_HDL, 64, testArray);
}
