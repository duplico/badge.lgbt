/******************************************************************************

 @file  urfi.h

 @brief This file contains the RF Interface.

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

#ifndef URFI_H
#define URFI_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <driverlib/rf_ble_cmd.h>

/*********************************************************************
 * CONSTANTS
 */

/* Invalid RF command handle */
#define URFI_CMD_HANDLE_INVALID   RF_ALLOC_ERROR

/*********************************************************************
 * TYPEDEFS
 */


/* There are four different types of command struct depending on ADV type.
   However, they are actually all the same. We can use a common struct.
*/
typedef rfc_CMD_BLE_ADV_t  rfc_CMD_BLE_ADV_COMMON_t;


/*****************************************************************************
 * FUNCTION PROTOTYPES
 */

/**
 * @brief  Initialize RF Interface for Micro BLE Stack
 *
 * @param  none
 *
 * @return SUCCESS - RF driver opened successfully
 *         INVALIDPARAMETER - Failed to open RF driver
 */
bStatus_t urfi_init(void);

/*********************************************************************
 * @fn      urfi_getTxPowerVal
 *
 * @brief   Get the value, corresponding with the given TX Power,
 *          to be used to setup the radio accordingly.
 *
 * @param   txPower - TX Power in dBm.
 *
 * @return  The register value correspondign with txPower, if found.
 *          UBLE_TX_POWER_INVALID otherwise.
 */
uint16 urfi_getTxPowerVal(int8 dBm);

/*********************************************************************
 *  EXTERNAL VARIABLES
 */

/* RF driver handle */
extern RF_Handle    urfiHandle;
#if defined(FEATURE_ADVERTISER)
/* ADV command handle */
extern RF_CmdHandle urfiAdvHandle;
/* ADV command parameter */
extern rfc_bleAdvPar_t urfiAdvParams;
#endif /* FEATURE_ADVERTISER */

#if defined(FEATURE_SCANNER)
/* Scan command handle */
extern RF_CmdHandle urfiScanHandle;
/* Scan command parameter */
extern rfc_bleScannerPar_t urfiScanParams;
#endif /* FEATURE_SCANNER */

#if defined(FEATURE_MONITOR)
/* Monitor command handle */
extern RF_CmdHandle urfiGenericRxHandle;
/* Monitor command parameter */
extern rfc_bleGenericRxPar_t urfiGenericRxParams;
#endif /* FEATURE_MONITOR */

#ifdef __cplusplus
}
#endif

#endif /* URFI_H */
