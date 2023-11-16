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

#include "svc_mcs.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Characteristic read permissions */
#ifndef MCS_SEC_PERMIT_READ
#define MCS_SEC_PERMIT_READ SVC_SEC_PERMIT_READ
#endif

/*! Characteristic write permissions */
#ifndef MCS_SEC_PERMIT_WRITE
#define MCS_SEC_PERMIT_WRITE SVC_SEC_PERMIT_WRITE
#endif

/**************************************************************************************************
 Service variables
**************************************************************************************************/

/*Service variables declaration*/
const uint8_t attMcsSvcUuid[ATT_128_UUID_LEN] = { ATT_UUID_MCS_SERVICE };

/*Characteristic variables declaration*/
const uint8_t svcMcsButtonUuid[ATT_128_UUID_LEN] = { ATT_UUID_MCS_BUTTON };
const uint8_t svcMcsRUuid[ATT_128_UUID_LEN] = { ATT_UUID_MCS_R };
const uint8_t svcMcsGUuid[ATT_128_UUID_LEN] = { ATT_UUID_MCS_G };
const uint8_t svcMcsBUuid[ATT_128_UUID_LEN] = { ATT_UUID_MCS_B };
const uint8_t svcMcsXUuid[ATT_128_UUID_LEN] = { ATT_UUID_MCS_X };
const uint8_t svcMcsCancelTimerUuid[ATT_128_UUID_LEN] = { ATT_UUID_MCS_CANCEL_TIMER };
const uint8_t svcMcsOnRainUuid[ATT_128_UUID_LEN] = { ATT_UUID_MCS_ON_RAIN };
//const uint8_t svcMcsOnCapUuid[ATT_128_UUID_LEN] = { ATT_UUID_MCS_ON_CAP };


static const uint8_t mcsValSvc[] = { ATT_UUID_MCS_SERVICE };
static const uint16_t mcsLenSvc = sizeof(mcsValSvc);

static const uint8_t mcsButtonValCh[] = { ATT_PROP_READ | ATT_PROP_NOTIFY,
                                          UINT16_TO_BYTES(MCS_BUTTON_HDL), ATT_UUID_MCS_BUTTON };
static const uint16_t mcsButtonLenCh = sizeof(mcsButtonValCh);

static const uint8_t mcsRValCh[] = { ATT_PROP_READ | ATT_PROP_WRITE, UINT16_TO_BYTES(MCS_ON_OFF_HDL),
                                     ATT_UUID_MCS_R };
static const uint16_t mcsRLenCh = sizeof(mcsRValCh);

static const uint8_t mcsGValCh[] = { ATT_PROP_READ | ATT_PROP_WRITE, UINT16_TO_BYTES(MCS_SENSOR_HDL),
                                     ATT_UUID_MCS_G };
static const uint16_t mcsGLenCh = sizeof(mcsGValCh);

static const uint8_t mcsBValCh[] = { ATT_PROP_READ | ATT_PROP_WRITE, UINT16_TO_BYTES(MCS_ROOT_HDL),
                                     ATT_UUID_MCS_B };
static const uint16_t mcsBLenCh = sizeof(mcsBValCh);

static const uint8_t mcsXValCh[] = { ATT_PROP_READ | ATT_PROP_WRITE, UINT16_TO_BYTES(MCS_SCHEDULE_HDL),
                                     ATT_UUID_MCS_X };
static const uint16_t mcsXLenCh = sizeof(mcsXValCh);

static const uint8_t mcsCancelTimerCh[] = { ATT_PROP_READ | ATT_PROP_WRITE, UINT16_TO_BYTES(MCS_CANCEL_TIMER_HDL),
                                     ATT_UUID_MCS_CANCEL_TIMER };
static const uint16_t mcsCancelTimerLenCh = sizeof(mcsCancelTimerCh);

static const uint8_t mcsOnRainCh[] = { ATT_PROP_READ | ATT_PROP_WRITE, UINT16_TO_BYTES(MCS_ON_RAIN_HDL),
                                     ATT_UUID_MCS_ON_RAIN };
static const uint16_t mcsOnRainLenCh = sizeof(mcsOnRainCh);

#if 0
static const uint8_t mcsOnCapCh[] = { ATT_PROP_READ | ATT_PROP_WRITE, UINT16_TO_BYTES(MCS_ON_CAP_HDL),
                                     ATT_UUID_MCS_ON_CAP };
static const uint16_t mcsOnCapLenCh = sizeof(mcsOnCapCh);
#endif



/*Characteristic values declaration*/
static uint8_t mcsButtonVal[] = { 0 };
static const uint16_t mcsButtonValLen = sizeof(mcsButtonVal);

static uint8_t mcsButtonValChCcc[] = { UINT16_TO_BYTES(0x0000) };
static const uint16_t mcsButtonLenValChCcc = sizeof(mcsButtonValChCcc);

static uint8_t mcsRVal[] = { 0 };
static const uint16_t mcsRValLen = sizeof(mcsRVal);

static uint8_t mcsGVal[] = { 0 };
static const uint16_t mcsGValLen = sizeof(mcsGVal);

static uint8_t mcsBVal[] = { 0 };
static const uint16_t mcsBValLen = sizeof(mcsBVal);

static uint8_t mcsXVal[4] = { 0 , 0 , 0 , 0 };
static const uint16_t mcsXValLen = sizeof(mcsXVal);

static uint8_t mcsCancelTimer[] = { 0 };
static const uint16_t mcsCancelTimerLen = sizeof(mcsCancelTimer);

static uint8_t mcsOnRain[] = { 0 };
static const uint16_t mcsOnRainLen = sizeof(mcsOnRain);

#if 0
static uint8_t mcsOnCap[] = { 0 };
static const uint16_t mcsOnCapLen = sizeof(mcsOnCap);
#endif

/**************************************************************************************************
 Maxim Custom Service group
**************************************************************************************************/

/* Attribute list for mcs group */
static const attsAttr_t mcsList[] = {
    /*-----------------------------*/
    /* Service declaration */
    { attPrimSvcUuid, (uint8_t *)mcsValSvc, (uint16_t *)&mcsLenSvc, sizeof(mcsValSvc), 0,
      MCS_SEC_PERMIT_READ },

    /*----------------------------*/
    /* Button characteristic declaration */
    { attChUuid, (uint8_t *)mcsButtonValCh, (uint16_t *)&mcsButtonLenCh, sizeof(mcsButtonValCh),
    ATTS_SET_WRITE_CBACK, MCS_SEC_PERMIT_READ},
    /* Button characteristic value */
    { svcMcsButtonUuid, (uint8_t *)mcsButtonVal, (uint16_t *)&mcsButtonValLen, sizeof(mcsButtonVal),
    ATTS_SET_WRITE_CBACK, (MCS_SEC_PERMIT_READ | MCS_SEC_PERMIT_WRITE) },
    /*Button characteristic value descriptor*/
    { attCliChCfgUuid, (uint8_t *)mcsButtonValChCcc, (uint16_t *)&mcsButtonLenValChCcc,
      sizeof(mcsButtonValChCcc), ATTS_SET_CCC, (ATTS_PERMIT_READ | SVC_SEC_PERMIT_WRITE) },

    /*-----------------------------*/
    /* R characteristic declaration */
    { attChUuid, (uint8_t *)mcsRValCh, (uint16_t *)&mcsRLenCh, sizeof(mcsRValCh),
      ATTS_SET_WRITE_CBACK, (MCS_SEC_PERMIT_READ | MCS_SEC_PERMIT_WRITE) },
    /* R characteristic characteristic value */
    { svcMcsRUuid, (uint8_t *)mcsRVal, (uint16_t *)&mcsRValLen, sizeof(mcsRVal),
      ATTS_SET_WRITE_CBACK, (MCS_SEC_PERMIT_READ | MCS_SEC_PERMIT_WRITE) },

    /*-----------------------------*/
    /* G characteristic declaration */
    { attChUuid, (uint8_t *)mcsGValCh, (uint16_t *)&mcsGLenCh, sizeof(mcsGValCh),
      ATTS_SET_WRITE_CBACK, (MCS_SEC_PERMIT_READ | MCS_SEC_PERMIT_WRITE) },
    /* G characteristic characteristic value */
    { svcMcsGUuid, (uint8_t *)mcsGVal, (uint16_t *)&mcsGValLen, sizeof(mcsGVal),
      ATTS_SET_WRITE_CBACK, (MCS_SEC_PERMIT_READ | MCS_SEC_PERMIT_WRITE) },

    /*-----------------------------*/
    /*  B characteristic declaration */
    { attChUuid, (uint8_t *)mcsBValCh, (uint16_t *)&mcsBLenCh, sizeof(mcsBValCh),
      ATTS_SET_WRITE_CBACK, (MCS_SEC_PERMIT_READ | MCS_SEC_PERMIT_WRITE) },
    /* B characteristic value */
    { svcMcsBUuid, (uint8_t *)mcsBVal, (uint16_t *)&mcsBValLen, sizeof(mcsBVal),
      ATTS_SET_WRITE_CBACK, (MCS_SEC_PERMIT_READ | MCS_SEC_PERMIT_WRITE) },

    /*-----------------------------*/
    /*  X characteristic declaration */
    { attChUuid, (uint8_t *)mcsXValCh, (uint16_t *)&mcsXLenCh, sizeof(mcsXValCh),
      ATTS_SET_WRITE_CBACK, (MCS_SEC_PERMIT_READ | MCS_SEC_PERMIT_WRITE) },
    /* X characteristic value */
    { svcMcsXUuid, (uint8_t *)mcsXVal, (uint16_t *)&mcsXValLen, sizeof(mcsXVal),
      ATTS_SET_WRITE_CBACK, (MCS_SEC_PERMIT_READ | MCS_SEC_PERMIT_WRITE) },

    /*-----------------------------*/
    /*  X characteristic declaration */
    { attChUuid, (uint8_t *)mcsCancelTimerCh, (uint16_t *)&mcsCancelTimerLenCh, sizeof(mcsCancelTimerCh),
      ATTS_SET_WRITE_CBACK, (MCS_SEC_PERMIT_READ | MCS_SEC_PERMIT_WRITE) },
    /* X characteristic value */
    { svcMcsCancelTimerUuid, (uint8_t *)mcsCancelTimer, (uint16_t *)&mcsCancelTimerLen, sizeof(mcsCancelTimer),
      ATTS_SET_WRITE_CBACK, (MCS_SEC_PERMIT_READ | MCS_SEC_PERMIT_WRITE) },

    { attChUuid, (uint8_t *)mcsOnRainCh, (uint16_t *)&mcsOnRainLenCh, sizeof(mcsOnRainCh),
      ATTS_SET_WRITE_CBACK, (MCS_SEC_PERMIT_READ | MCS_SEC_PERMIT_WRITE) },
    /* X characteristic value */
    { svcMcsOnRainUuid, (uint8_t *)mcsOnRain, (uint16_t *)&mcsOnRainLen, sizeof(mcsOnRain),
      ATTS_SET_WRITE_CBACK, (MCS_SEC_PERMIT_READ | MCS_SEC_PERMIT_WRITE) }
#if 0
    { attChUuid, (uint8_t *)mcsOnCapCh, (uint16_t *)&mcsOnCapLenCh, sizeof(mcsOnCapCh),
      ATTS_SET_WRITE_CBACK, (MCS_SEC_PERMIT_READ | MCS_SEC_PERMIT_WRITE) },
    /* X characteristic value */
    { svcMcsOnCapUuid, (uint8_t *)mcsOnCap, (uint16_t *)&mcsOnCapLen, sizeof(mcsOnCap),
      ATTS_SET_WRITE_CBACK, (MCS_SEC_PERMIT_READ | MCS_SEC_PERMIT_WRITE) }
#endif
};

/* Test group structure */
static attsGroup_t svcMcsGroup = { NULL, (attsAttr_t *)mcsList, NULL,
                                   NULL, MCS_START_HDL,         MCS_END_HDL };

/*************************************************************************************************/
/*!
 *  \brief  Add the services to the attribute server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcMcsAddGroup(void)
{
    AttsAddGroup(&svcMcsGroup);
}

/*************************************************************************************************/
/*!
 *  \brief  Remove the services from the attribute server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcMcsRemoveGroup(void)
{
    AttsRemoveGroup(MCS_START_HDL);
}

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
void SvcMcsCbackRegister(attsReadCback_t readCback, attsWriteCback_t writeCback)
{
    svcMcsGroup.readCback = readCback;
    svcMcsGroup.writeCback = writeCback;
}
