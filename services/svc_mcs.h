/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Maxim Custom service server.
 *
 *  Copyright (c) 2012-2018 Arm Ltd. All Rights Reserved.
 *  ARM Ltd. confidential and proprietary.
 *
 *  IMPORTANT.  Your use of this file is governed by a Software License Agreement
 *  ("Agreement") that must be accepted in order to download or otherwise receive a
 *  copy of this file.  You may not use or copy this file for any purpose other than
 *  as described in the Agreement.  If you do not agree to all of the terms of the
 *  Agreement do not use this file and delete all copies in your possession or control;
 *  if you do not have a copy of the Agreement, you must contact ARM Ltd. prior
 *  to any use, copying or further distribution of this software.
 */
/*************************************************************************************************/

#ifndef EXAMPLES_MAX32655_BLE_MCS_SERVICES_SVC_MCS_H_
#define EXAMPLES_MAX32655_BLE_MCS_SERVICES_SVC_MCS_H_

#include "wsf_types.h"
#include "att_api.h"
#include "util/bstream.h"
#include "svc_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! \addtogroup Mcs
 *  \{ */
/**************************************************************************************************
  Macros
**************************************************************************************************/
/*MCS service UUID*/
#define ATT_UUID_MCS_SERVICE                                                                  \
    0xBE, 0xC5, 0xD1, 0x24, 0x99, 0x33, 0xC6, 0x87, 0x85, 0x41, 0xD9, 0x31, 0x7D, 0x56, 0xFC, \
        0x85 /*!< \brief Test Service UUID*/

/* MCS service GATT characteristic UUIDs*/
#define ATT_UUID_MCS_BUTTON \
    0xBE, 0xC5, 0xD1, 0x24, 0x99, 0x33, 0xC6, 0x87, 0x85, 0x41, 0xD9, 0x31, 0x7E, 0x56, 0xFC, 0x85
#define ATT_UUID_MCS_R \
    0xBE, 0xC5, 0xD1, 0x24, 0x99, 0x33, 0xC6, 0x87, 0x85, 0x41, 0xD9, 0x31, 0x7F, 0x56, 0xFC, 0x85
#define ATT_UUID_MCS_G \
    0xBE, 0xC5, 0xD1, 0x24, 0x99, 0x33, 0xC6, 0x87, 0x85, 0x41, 0xD9, 0x31, 0x80, 0x56, 0xFC, 0x85
#define ATT_UUID_MCS_B \
    0xBE, 0xC5, 0xD1, 0x24, 0x99, 0x33, 0xC6, 0x87, 0x85, 0x41, 0xD9, 0x31, 0x81, 0x56, 0xFC, 0x85
#define ATT_UUID_MCS_X \
    0xBE, 0xC5, 0xD1, 0x24, 0x99, 0x33, 0xC6, 0x87, 0x85, 0x41, 0xD9, 0x31, 0x82, 0x56, 0xFC, 0x85
#define ATT_UUID_MCS_CANCEL_TIMER \
    0xBE, 0xC5, 0xD1, 0x24, 0x99, 0x33, 0xC6, 0x87, 0x85, 0x41, 0xD9, 0x31, 0x83, 0x56, 0xFC, 0x85
#define ATT_UUID_MCS_ON_RAIN \
    0xBE, 0xC5, 0xD1, 0x24, 0x99, 0x33, 0xC6, 0x87, 0x85, 0x41, 0xD9, 0x31, 0x84, 0x56, 0xFC, 0x85
#if 0
#define ATT_UUID_MCS_ON_CAP \
    0xBE, 0xC5, 0xD1, 0x24, 0x99, 0x33, 0xC6, 0x87, 0x85, 0x41, 0xD9, 0x31, 0x85, 0x56, 0xFC, 0x85
#endif

/**************************************************************************************************
 Handle Ranges
**************************************************************************************************/

/** \name Maxim custom Service Handles
 *
 */
/**@{*/
#define MCS_START_HDL 0x1500 /*!< \brief Start handle. */
#define MCS_END_HDL (MCS_MAX_HDL - 1) /*!< \brief End handle. */

/**************************************************************************************************
 Handles
**************************************************************************************************/

/*! \brief Maxim custom Service Handles */
enum {
    MCS_SVC_HDL = MCS_START_HDL, /*!< \brief Maxim custom service declaration */
    MCS_BUTTON_CH_HDL, /*!< \brief Button characteristic */
    MCS_BUTTON_HDL, /*!< \brief Button*/
    MCS_BUTTON_CH_CCC_HDL, /*!< \brief Button CCCD*/
    MCS_ON_OFF_CH_HDL, /*!< \brief R characteristic */
    MCS_ON_OFF_HDL, /*!< \brief R*/
    MCS_SENSOR_CH_HDL, /*!< \brief G characteristic */
    MCS_SENSOR_HDL, /*!< \brief G*/
    MCS_ROOT_CH_HDL, /*!< \brief B characteristic */
    MCS_ROOT_HDL, /*!< \brief B*/
    MCS_SCHEDULE_CH_HDL, /*!< \brief B characteristic */
    MCS_SCHEDULE_HDL, /*!< \brief B*/
    MCS_CANCEL_TIMER_CH_HDL,
    MCS_CANCEL_TIMER_HDL,
    MCS_ON_RAIN_CH_HDL,
    MCS_ON_RAIN_HDL, 
    //MCS_ON_CAP_CH_HDL,
    //MCS_ON_CAP_HDL,  
    MCS_MAX_HDL /*!< \brief Maximum handle. */
};
/**@}*/

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Add the services to the attribute server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcMcsAddGroup(void);

/*************************************************************************************************/
/*!
 *  \brief  Remove the services from the attribute server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcMcsRemoveGroup(void);

/*************************************************************************************************/
/*!
 *  \brief  Register callbacks for the service.
 *
 *  \param  readCback   Read callback function.
 *  \param  writeCback  Write callback function.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcMcsCbackRegister(attsReadCback_t readCback, attsWriteCback_t writeCback);

/*! \} */ /* TEST_SERVICE */

#ifdef __cplusplus
};
#endif

#endif // EXAMPLES_MAX32655_BLE_MCS_SERVICES_SVC_MCS_H_
