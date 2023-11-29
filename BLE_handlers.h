#ifndef BLE_HANDLERS_H
#define BLE_HANDLERS_H

#include "app_ui.h"

#if 0
enum {
    MANUAL_ON = 1,
    MANUAL_OFF,
    MANUAL_CANCEL_TIME
};
#endif

void manualOnOffHandler(uint16_t handle, uint8_t *pValue);

void scheduleArrayHandler(uint16_t len, uint8_t *pValue);

void rootDepthHandler(uint8_t *pValue);

void onSensorSet( uint8_t *pValue);

void loadData(uint8_t *pValue);

void requestData();







#endif


