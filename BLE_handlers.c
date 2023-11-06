#include <stdbool.h>
#include <stdio.h>
#include "app_ui.h"
#include "mcs_api.h"
#define MANUAL_TIMER MXC_TMR0


extern int manualOff;
extern int manualOn;
extern int manualTime;
extern int rootDepth;
extern int scheduleTimeArray[8];

void manualOffHandler(uint16_t handle)
{
    //if (currentState = )
    manualOff = 1;
    manualOn = 0;
    manualTime = 1;
    printf("manual Off = 1: %u", manualOff);
    printf("\n");
    fflush(stdout);

    AttsSetAttr(handle, 1, 0);
    MXC_TMR_Shutdown(MANUAL_TIMER);
    MXC_TMR_Start(MANUAL_TIMER);

    //MXC_delay(3000000);
}

void manualOnHandler(uint16_t handle)
{
    //if (currentState = )
    manualOn = 1;
    manualOff = 0;
    manualTime = 1;
    printf("manual On = 1: %u", manualOn);
    printf("\n");
    fflush(stdout);

    AttsSetAttr(handle, 1, 0);
    MXC_TMR_Shutdown(MANUAL_TIMER);
    MXC_TMR_Start(MANUAL_TIMER);


    //MXC_delay(3000000);
}

void scheduleArrayHandler(uint16_t len, uint8_t *pValue)
{
    int i = 0;
    for (i = 0; i < len; i++){
        printf("pValue test: %u \n \n", *(pValue + i));
        scheduleTimeArray[i] = *(pValue + i);
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