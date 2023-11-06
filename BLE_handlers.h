#ifndef BLE_HANDLERS_H
#define BLE_HANDLERS_H

#include "app_ui.h"




void manualOffHandler(uint16_t handle);

void manualOnHandler(uint16_t handle);

void scheduleArrayHandler(uint16_t len, uint8_t *pValue);

void rootDepthHandler(uint8_t *pValue);









#endif


