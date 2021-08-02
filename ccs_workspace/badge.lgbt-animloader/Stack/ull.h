/******************************************************************************

 @file  ull.h

 @brief This file contains the Micro Link Layer (uLL) API for the Micro
        BLE Stack.

 Group: WCS, BTS
 Target Device: cc2640r2

 ******************************************************************************
 
 Copyright (c) 2009-2019, Texas Instruments Incorporated
 All rights reserved.

 IMPORTANT: Your use of this Software is limited to those specific rights
 granted under the terms of a software license agreement between the user
 who downloaded the software, his/her employer (which must be your employer)
 and Texas Instruments Incorporated (the "License"). You may not use this
 Software unless you agree to abide by the terms of the License. The License
 limits your use, and you acknowledge, that the Software may not be modified,
 copied or distributed unless embedded on a Texas Instruments microcontroller
 or used solely and exclusively in conjunction with a Texas Instruments radio
 frequency transceiver, which is integrated into your product. Other than for
 the foregoing purpose, you may not use, reproduce, copy, prepare derivative
 works of, modify, distribute, perform, display or sell this Software and/or
 its documentation for any purpose.

 YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
 PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
 NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
 TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
 NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
 LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
 INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
 OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
 OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
 (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

 Should you have any questions regarding your right to use this Software,
 contact Texas Instruments Incorporated at www.TI.com.

 ******************************************************************************
 
 
 *****************************************************************************/

#ifndef ULL_H
#define ULL_H

#ifdef __cplusplus
extern "C"
{
#endif

/*****************************************************************************
* INCLUDES
*/

/*-------------------------------------------------------------------
 * CONSTANTS
 */

/**
 * Link layer Advertiser Events
 */
#define ULL_EVT_ADV_TX_NO_RADIO_RESOURCE    1 //!< Adv event no radio resource
#define ULL_EVT_ADV_TX_FAILED               2 //!< Adv event tx failed
#define ULL_EVT_ADV_TX_SUCCESS              3 //!< Adv event tx success
#define ULL_EVT_ADV_TX_RADIO_AVAILABLE      4 //!< Adv event radio available
#define ULL_EVT_ADV_TX_TIMER_EXPIRED        5 //!< Adv event interval timer expired
#define ULL_EVT_ADV_TX_STARTED              6 //!< Adv event tx started
   
/**
 * Link layer Scanner Events
 */
#define ULL_EVT_SCAN_RX_NO_RADIO_RESOURCE   7 //!< Scan event rx no radio resource
#define ULL_EVT_SCAN_RX_FAILED              8 //!< Scan event rx failed
#define ULL_EVT_SCAN_RX_BUF_FULL            9 //!< Scan event rx buffer full
#define ULL_EVT_SCAN_RX_SUCCESS            10 //!< Scan event rx success
#define ULL_EVT_SCAN_RX_RADIO_AVAILABLE    11 //!< Scan event radio available
#define ULL_EVT_SCAN_RX_STARTED            12 //!< Scan event rx started

/**
 * Link layer Monitor Events
 */
#define ULL_EVT_MONITOR_RX_NO_RADIO_RESOURCE  13 //!< Monitor event rx no radio resource
#define ULL_EVT_MONITOR_RX_FAILED             14 //!< Monitor event rx failed
#define ULL_EVT_MONITOR_RX_BUF_FULL           15 //!< Monitor event rx buffer full   
#define ULL_EVT_MONITOR_RX_SUCCESS            16 //!< Monitor event rx success
#define ULL_EVT_MONITOR_RX_WINDOW_COMPLETE    17 //!< Monitor event rx window complete
#define ULL_EVT_MONITOR_RX_RADIO_AVAILABLE    18 //!< Monitor event radio available
#define ULL_EVT_MONITOR_RX_STARTED            19 //!< Monitor event rx started
   
/**
 * Link layer Scanner number of rx entries
 */
#define ULL_NUM_RX_SCAN_ENTRIES             6 //!< Number of scan rx entries
#define ULL_PKT_HDR_LEN                     2 //!< Packet hearde length
#define ULL_MAX_BLE_ADV_PKT_SIZE           37 //!< Payload size

/**
 * Link layer Monitor number of rx entries
 */
#define ULL_NUM_RX_MONITOR_ENTRIES          6 //!< Number of monitoring scan rx entries
#define ULL_PKT_HDR_LEN                     2 //!< Packet header length
#define ULL_MAX_BLE_PKT_SIZE              255 //!< Payload size.


/**
 * Link layer Receive Suffix Data Sizes
 */
#define ULL_SUFFIX_CRC_SIZE             3
#define ULL_SUFFIX_RSSI_SIZE            1
#define ULL_SUFFIX_STATUS_SIZE          1
#define ULL_SUFFIX_TIMESTAMP_SIZE       4
#define ULL_SUFFIX_MAX_SIZE             (ULL_SUFFIX_CRC_SIZE    +              \
                                         ULL_SUFFIX_RSSI_SIZE   +              \
                                         ULL_SUFFIX_STATUS_SIZE +              \
                                         ULL_SUFFIX_TIMESTAMP_SIZE)

#define ULL_ADV_UNALIGNED_BUFFER_SIZE   (ULL_PKT_HDR_LEN + ULL_MAX_BLE_ADV_PKT_SIZE + ULL_SUFFIX_MAX_SIZE)
#define ULL_ADV_ALIGNED_BUFFER_SIZE     (ULL_ADV_UNALIGNED_BUFFER_SIZE + 4 - \
                                         (ULL_ADV_UNALIGNED_BUFFER_SIZE % 4))

#define ULL_BLE_UNALIGNED_BUFFER_SIZE   (ULL_PKT_HDR_LEN + ULL_MAX_BLE_PKT_SIZE + ULL_SUFFIX_MAX_SIZE)
#define ULL_BLE_ALIGNED_BUFFER_SIZE     (ULL_BLE_UNALIGNED_BUFFER_SIZE + 4 - \
                                         (ULL_BLE_UNALIGNED_BUFFER_SIZE % 4))
 
/** 
 * Scanner White List Policy
 */
#define ULL_SCAN_WL_POLICY_ANY_ADV_PKTS                0
#define ULL_SCAN_WL_POLICY_USE_WHITE_LIST              1


/******************************************************************************
* TYPEDEFS
*/

#if defined(FEATURE_ADVERTISER)

/**
 * Callback of when an Adv event is about to start.
 */
typedef void (*pfnAdvAboutToCB_t)(void);

/**
 * Callback of when events come from RF driver regarding Adv command.
 */
typedef void (*pfnAdvDoneCB_t)(bStatus_t status);

#endif /* FEATURE_ADVERTISER */

#if defined(FEATURE_SCANNER)

/**
 * Callback of when a scan has received a packet.
 */
typedef void (*pfnScanIndCB_t)(bStatus_t status, uint8_t len, uint8_t *pPayload);

/**
 * Callback of when events when a scan wondow is complete.
 */
typedef void (*pfnScanWindowCompCB_t)(bStatus_t status);

/**
 * Flow control to allow one RX packet at a time
 */
extern bool Ull_advPktInuse;

#endif /* FEATURE_SCANNER */

#if defined(FEATURE_MONITOR)

/**
 * Callback of when a monitoring scan has received a packet.
 */
typedef void (*pfnMonitorIndCB_t)(bStatus_t status, uint8_t sessionId, uint8_t len, uint8_t *pPayload);

/**
 * Callback of when a monitor duration is complete.
 */
typedef void (*pfnMonitorCompCB_t)(bStatus_t status, uint8_t sessionId);

#endif /* FEATURE_MONITOR */


/*****************************************************************************
 * FUNCTION PROTOTYPES
 */

/*********************************************************************
 * @fn     ull_init
 *
 * @brief  Initialization function for the Micro Link Layer.
 *
 * @param  none
 *
 * @return SUCCESS or FAILURE
 */
bStatus_t ull_init(void);

#if defined(FEATURE_ADVERTISER)
/*********************************************************************
 * @fn     ull_advRegisterCB
 *
 * @brief  Register callbacks supposed to be called by Advertiser.
 *
 * @param  pfnAdvAboutToCB - callback to nofity the application of time to
 *                           update the advertising packet payload.
 * @param  pfnAdvDoneCB    - callback to post-process Advertising Event
 *
 * @return  none
 */
void ull_advRegisterCB(pfnAdvAboutToCB_t pfnAdvAboutToCB,
                       pfnAdvDoneCB_t pfnAdvDoneCB);

/*********************************************************************
 * @fn      ull_advStart
 *
 * @brief   Enter ULL_STATE_ADVERTISING
 *
 * @param   none
 *
 * @return  SUCCESS - Successfully entered ULL_STATE_ADVERTISING
 *          FAILURE - Failed to enter ULL_STATE_ADVERSISING
 */
bStatus_t ull_advStart(void);

/*********************************************************************
 * @fn      ull_advStop
 *
 * @brief   Exit ULL_STATE_ADVERTISING
 *
 * @param   none
 *
 * @return  none
 */
void      ull_advStop(void);

#endif /* FEATURE_ADVERTISER */

#if defined(FEATURE_SCANNER)
/*********************************************************************
 * @fn     ull_scanRegisterCB
 *
 * @brief  Register callbacks supposed to be called by Scanner.
 *
 * @param  pfnScanIndicationCB - callback to nofity the application that a packet is
 *                               received.
 * @param  pfnScanWindowCompleteCB - callback to post-process scan window complete Event
 *
 * @return  none
 */
void ull_scanRegisterCB(pfnScanIndCB_t pfnScanIndicationCB,
                        pfnScanWindowCompCB_t pfnScanWindowCompleteCB);

/*********************************************************************
 * @fn      ull_scanStart
 *
 * @brief   Enter ULL_STATE_SCANNING
 *
 * @param   scanChanIndex - scan channel index
 *
 * @return  SUCCESS - Successfully entered ULL_STATE_SCANNING
 *          FAILURE - Failed to enter ULL_STATE_SCANNING
 */
bStatus_t ull_scanStart(uint8_t scanChanIndexMap);

/*********************************************************************
 * @fn      ull_scanStop
 *
 * @brief   Exit ULL_STATE_SCANNING
 *
 * @param   none
 *
 * @return  none
 */
void      ull_scanStop(void);

#endif /* FEATURE_SCANNER */

#if defined(FEATURE_MONITOR)
/*********************************************************************
 * @fn     ull_monitorRegisterCB
 *
 * @brief  Register callbacks supposed to be called by Monitor.
 *
 * @param  pfnMonitorIndicationCB - callback to nofity the application that
 *                                  a packet is received.
 * @param  pfnMonitorCompleteCB - callback to post-process monitor duration complete Event
 *
 * @return  none
 */
void ull_monitorRegisterCB(pfnMonitorIndCB_t pfnMonitorIndicationCB,
                           pfnMonitorCompCB_t pfnMonitorCompleteCB);

/*********************************************************************
 * @fn      ull_monitorStart
 *
 * @brief   Enter ULL_STATE_MONITORING
 *
 * @param   channel - monitoring scan channel
 *
 * @return  SUCCESS - Successfully entered ULL_STATE_MONITORING
 *          FAILURE - Failed to enter ULL_STATE_MONITORING
 */
bStatus_t ull_monitorStart(uint8_t channel);

/*********************************************************************
 * @fn      ull_monitorStop
 *
 * @brief   Exit ULL_STATE_SMONITORING
 *
 * @param   none
 *
 * @return  none
 */
void      ull_monitorStop(void);

#endif /* FEATURE_MONITOR */

#ifdef __cplusplus
}
#endif

#endif /* ULL_H */
