#include <stdbool.h>
#include <stdio.h>
#include "app_ui.h"
#include "mcs_api.h"
#include "tmr.h"
#include "solenoid_fun.h"
#define MANUAL_TIMER MXC_TMR4
#define TAKE_SAMPLE_TIMER MXC_TMR1


extern int manualOff;
extern int manualOn;
extern int manualTime;
extern int rootDepth;
extern int scheduleTimeArray[8];
extern int rainOn;
extern int capOn;

extern int solenoidState;

void manualOnOffHandler(uint16_t handle, uint8_t *pValue)
{
    printf("sent value: %u \n", *pValue);
    fflush(stdout);

    switch(*pValue){
        case 1: //Manual ON
        
            manualOn = 1;
            manualOff = 0;
            manualTime = 1;
            //MXC_TMR_Shutdown(MANUAL_TIMER);
            //MXC_TMR_Start(MANUAL_TIMER);

            solenoidState = 1;
            //solenoidInit();
            openSolenoid();

            printf("Manual On \n");
            fflush(stdout);

            break;
        case 2:  //Manual Off

            manualOn = 0;
            manualOff = 1;
            manualTime = 1;
            //MXC_TMR_Shutdown(MANUAL_TIMER);
            //MXC_TMR_Start(MANUAL_TIMER);

            solenoidState = 0;
            //solenoidInit();
            closeSolenoid();

            printf("Manual Off \n");
            fflush(stdout);

            break;
        case 3:  //Cancel Timer
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
    int hour = (time/4)*100; 
    int minute = (time%4)*15;
    return hour+minute;

}

void scheduleArrayHandler(uint16_t len, uint8_t *pValue)
{
    int i = 0;
    printf("callback hit");
    fflush(stdout);
    for (i = 0; i < len; i++){
        //printf("pValue test: %u \n \n", *(pValue + i));
        scheduleTimeArray[i] = timeConverter(*(pValue + i));
        printf("Array test: %u \n", scheduleTimeArray[i]);
    }
    fflush(stdout);
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
        rainOn = 1;
        printf("rainOn: %u \n", rainOn);
        fflush(stdout);
        break;

        case 2: //rain off
        rainOn = 0;
        printf("rainOn: %u \n", rainOn);
        fflush(stdout);
        MXC_TMR_Shutdown(TAKE_SAMPLE_TIMER);
        break;

        case 3: //cap On
        capOn = 1;
        printf("capOn: %u \n", capOn);
        fflush(stdout);
        break;

        case 4: //cap Off
        capOn = 0;
        printf("capOn: %u \n", capOn);
        fflush(stdout);
        MXC_TMR_Shutdown(TAKE_SAMPLE_TIMER);
        break;
    }
}
