/******************************************************************************

 @file  urfi.c

 @brief This file contains the RF driver interfacing API for the Micro
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

/*********************************************************************
 * INCLUDES
 */
#include <driverlib/rf_mailbox.h>
#include <driverlib/rf_ble_cmd.h>
#include <driverlib/rf_ble_mailbox.h>
#include <driverlib/rf_common_cmd.h>
#include <ti/drivers/rf/RF.h>
#include <inc/hw_rfc_dbell.h>
#include <inc/hw_memmap.h>
#include <bcomdef.h>

#include <ll_common.h>
#include <port.h>
#include <urfi.h>
#include <uble.h>
#include <ull.h>

/*********************************************************************
 * CONSTANTS
 */

#if (!defined(RF_SINGLEMODE) && !defined(RF_MULTIMODE)) ||                   \
    (defined(RF_SINGLEMODE) && defined(RF_MULTIMODE))
  #error "Either RF_SINGLEMODE or RF_MULTIMODE should be defined."
#endif /* RF_SINGLEMODE, RF_MULTIMODE */
      
#if (defined(FEATURE_SCANNER) && defined(FEATURE_MONITOR))
  #error "FEATURE_SCANNER and FEATURE_MONITOR cannot be defined at the same time."
#endif /* FEATURE_SCANNER, FEATURE_MONITOR */      

/*********************************************************************
 * TYPEDEFS
 */

#ifdef RTLS_LOCATIONING_AOA
#define CMD_WRITE_FWPAR                0x000C
struct __RFC_STRUCT rfc_CMD_WRITE_FWPAR_s {
   uint16_t commandNo;
   uint16_t address;
   uint32_t value;
};

typedef struct __RFC_STRUCT rfc_CMD_WRITE_FWPAR_s rfc_CMD_WRITE_FWPAR_t;
#endif

/*********************************************************************
 * EXTERNAL VARIABLES
 */

extern ubleParams_t ubleParams;
extern uint8        ubleBDAddr[];
extern uint8        rfTimeCrit;
extern ubleAntSwitchCB_t ubleAntSwitchSel;

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */
extern bStatus_t uble_buildAndPostEvt(ubleEvtDst_t evtDst, ubleEvt_t evt,
                                      ubleMsg_t *pMsg, uint16 len);
extern dataEntryQ_t *ull_setupScanDataEntryQueue( void );
extern dataEntryQ_t *ull_setupMonitorDataEntryQueue( void );

/*********************************************************************
 * GLOBAL VARIABLES
 */

RF_Handle    urfiHandle = NULL;

/* Radio Setup Parameters.
 * config, txPower, and pRegOverride will be initialized at runtime.
 */
RF_RadioSetup urSetup =
{
  .common.commandNo                = CMD_RADIO_SETUP,
  .common.status                   = IDLE,
  .common.pNextOp                  = NULL,
  .common.startTime                = 0,
  .common.startTrigger.triggerType = TRIG_NOW,
  .common.startTrigger.bEnaCmd     = 0,
  .common.startTrigger.triggerNo   = 0,
  .common.startTrigger.pastTrig    = 0,
  .common.condition.rule           = COND_NEVER,
  .common.condition.nSkip          = 0,
  .common.mode                     = 0,
  .common.__dummy0                 = 0,
};

#if defined(FEATURE_ADVERTISER)
RF_CmdHandle urfiAdvHandle = URFI_CMD_HANDLE_INVALID;

/* CMD_BLE_ADV_XX Params */
rfc_bleAdvPar_t urfiAdvParams =
{
  .pRxQ = 0,
  .rxConfig.bAutoFlushIgnored = 0,
  .rxConfig.bAutoFlushCrcErr = 0,
  .rxConfig.bAutoFlushEmpty = 0,
  .rxConfig.bIncludeLenByte = 0,
  .rxConfig.bIncludeCrc = 0,
  .rxConfig.bAppendRssi = 0,
  .rxConfig.bAppendStatus = 0,
  .rxConfig.bAppendTimestamp = 0,
  .advConfig.advFilterPolicy = 0,
  .advConfig.deviceAddrType = 0,
  .advConfig.peerAddrType = 0,
  .advConfig.bStrictLenFilter = 0,
  .advLen = 0,
  .scanRspLen = 0,
  .pAdvData = ubleParams.advData,
#if defined(FEATURE_SCAN_RESPONSE)
  .pScanRspData = ubleParams.scanRspData,
#else   /* FEATURE_SCAN_RESPONSE */
  .pScanRspData = 0,
#endif  /* FEATURE_SCAN_RESPONSE */
  .pDeviceAddress = (uint16*) ubleBDAddr,
  .pWhiteList = 0,
  .__dummy0 = 0,
  .__dummy1 = 0,
  .endTrigger.triggerType = TRIG_NEVER,
  .endTrigger.bEnaCmd = 0,
  .endTrigger.triggerNo = 0,
  .endTrigger.pastTrig = 0,
  .endTime = 0,
};

#if defined(FEATURE_SCAN_RESPONSE)
/* CMD_BLE_ADV_XX Output */
rfc_bleAdvOutput_t urAdvOutput;
#endif  /* FEATURE_SCAN_RESPONSE */

/* CMD_BLE_ADV_XX Command */
rfc_CMD_BLE_ADV_COMMON_t urfiAdvCmd[3];

#endif  /* FEATURE_ADVERTISER */

#if defined(FEATURE_SCANNER)
RF_CmdHandle urfiScanHandle = URFI_CMD_HANDLE_INVALID;

/* CMD_BLE_SCANNER Params */
rfc_bleScannerPar_t urfiScanParams =
{
  .pRxQ = 0,
  .rxConfig.bAutoFlushIgnored = 1,
  .rxConfig.bAutoFlushCrcErr = 1,
  .rxConfig.bAutoFlushEmpty = 1,
  .rxConfig.bIncludeLenByte = 1,
  .rxConfig.bIncludeCrc = 0,
  .rxConfig.bAppendRssi = 1,
  .rxConfig.bAppendStatus = 1,
  .rxConfig.bAppendTimestamp = 1,
  .scanConfig.scanFilterPolicy = ULL_SCAN_WL_POLICY_ANY_ADV_PKTS,
  .scanConfig.bActiveScan = 0,  /* Passive scan only */
  .scanConfig.deviceAddrType = 0,
  .scanConfig.rpaFilterPolicy = 0,
  .scanConfig.bStrictLenFilter = 1,
  .scanConfig.bAutoWlIgnore = 0,
  .scanConfig.bEndOnRpt = 0, /* Must continue to scan */
  .scanConfig.rpaMode = 0,
  .randomState = 0,
  .backoffCount = 1,
  .backoffPar = 0,
  .scanReqLen = 0,
  .pScanReqData = 0,
  .pDeviceAddress = 0,
  .pWhiteList = 0,
  .__dummy0 = 0,
  .timeoutTrigger.triggerType = TRIG_NEVER,
  .timeoutTrigger.bEnaCmd = 0,
  .timeoutTrigger.triggerNo = 0,
  .timeoutTrigger.pastTrig = 1,
  .endTrigger.triggerType = TRIG_NEVER,
  .endTrigger.bEnaCmd = 0,
  .endTrigger.triggerNo = 0,
  .endTrigger.pastTrig = 0,
  .timeoutTime = 0,
  .endTime = 0,
};

/* CMD_BLE_SCANNER outputs */
rfc_bleScannerOutput_t pScanOutput;

/* CMD_BLE_SCANNER Command */
rfc_CMD_BLE_SCANNER_t urfiScanCmd;

#endif  /* FEATURE_SCANNER */

#if defined(FEATURE_MONITOR)
RF_CmdHandle urfiGenericRxHandle = URFI_CMD_HANDLE_INVALID;

/* CMD_BLE_GENERIC_RX Params */
rfc_bleGenericRxPar_t urfiGenericRxParams =
{
  .pRxQ = 0,
  .rxConfig.bAutoFlushIgnored = 0, // Should never turn on for generic Rx cmd
  .rxConfig.bAutoFlushCrcErr = 0, // Don't flush. Even if the packet is corrupt we still get a timestamp from it
  .rxConfig.bAutoFlushEmpty = 1,
  .rxConfig.bIncludeLenByte = 1,
  .rxConfig.bIncludeCrc = 0,
  .rxConfig.bAppendRssi = 1,
  .rxConfig.bAppendStatus = 1,
  .rxConfig.bAppendTimestamp = 1,
  .bRepeat = 1,
  .__dummy0 = 0,
  .accessAddress = 0,
  .crcInit0 = 0x55,
  .crcInit1 = 0x55,
  .crcInit2 = 0x55,
  .endTrigger.triggerType = TRIG_NEVER,
  .endTrigger.bEnaCmd = 0,
  .endTrigger.triggerNo = 0,
  .endTrigger.pastTrig = 1,
  .endTime = 0,
};

/* CMD_BLE_GENERIC_RX outputs */
rfc_bleGenericRxOutput_t pGenericRxOutput;

/* CMD_BLE_GENERIC_RX Command */
rfc_CMD_BLE_GENERIC_RX_t urfiGenericRxCmd;

#endif  /* FEATURE_MONITOR */

#ifdef RTLS_LOCATIONING_AOA
// CMD_WRITE_FWPAR
rfc_CMD_WRITE_FWPAR_t RF_cmdWriteFwParRx =
{
    .commandNo = CMD_WRITE_FWPAR,
    .address = 188 | (0 << 11),
    .value = 0,
};

// CMD_SET_RAT_CPT
// Set up RAT capture: RAT Channel 7, Rising edge, Single capture, and InputSrc 11
rfc_CMD_SET_RAT_CPT_t RF_cmdSetRatCpt =
{
    .commandNo = CMD_SET_RAT_CPT,
    .config.inputSrc = 11,
    .config.ratCh = 7,
    .config.bRepeated = 0,
    .config.inputMode = 0,
};

// CMD_SET_RAT_OUTPUT
// Setup IO configuration for RAT_GPO4, Pulse mode, RAT Channel 7
rfc_CMD_SET_RAT_OUTPUT_t RF_cmdSetRatOutput =
{
    .commandNo = CMD_SET_RAT_OUTPUT,
    .config.outputSel = 4,
    .config.outputMode = 0,
    .config.ratCh = 7,
};

// CMD_SCH_IMM
rfc_CMD_SCH_IMM_t RF_cmdSchImm2 =
{
    .commandNo = CMD_SCH_IMM,
    .status = 0x0000,
    .pNextOp = (rfc_radioOp_t *)&urfiGenericRxCmd,
    .startTime = 0x00000000,
    .startTrigger.triggerType = 0x0,
    .startTrigger.bEnaCmd = 0x0,
    .startTrigger.triggerNo = 0x0,
    .startTrigger.pastTrig = 0x0,
    .condition.rule = 0x0, // Always run next command
    .condition.nSkip = 0x0,
    .__dummy0 = 0,
    .cmdrVal = (uint32_t)&RF_cmdSetRatOutput,
    .cmdstaVal = 0x0000,
};

// CMD_SCH_IMM
rfc_CMD_SCH_IMM_t RF_cmdSchImm1 =
{
    .commandNo = CMD_SCH_IMM,
    .status = 0x0000,
    .pNextOp = (rfc_radioOp_t *)&RF_cmdSchImm2,
    .startTime = 0x00000000,
    .startTrigger.triggerType = 0x0,
    .startTrigger.bEnaCmd = 0x0,
    .startTrigger.triggerNo = 0x0,
    .startTrigger.pastTrig = 0x0,
    .condition.rule = 0x0, // Always run next command
    .condition.nSkip = 0x0,
    .__dummy0 = 0,
    .cmdrVal = (uint32_t)&RF_cmdSetRatCpt,
    .cmdstaVal = 0x0000,
};

// CMD_BLE_NOP
rfc_CMD_NOP_t RF_cmdNopRx =
{
    .commandNo = CMD_NOP,
    .status = 0x0000,
    .pNextOp = (rfc_radioOp_t *)&RF_cmdSchImm1,
    .startTime = 0x00000000,
    .startTrigger.triggerType = 0x0,
    .startTrigger.bEnaCmd = 0x0,
    .startTrigger.triggerNo = 0x0,
    .startTrigger.pastTrig = 0x0,
    .condition.rule = 0x0, // Always run next command
    .condition.nSkip = 0x0,
};


// Callback to turn on all clocks when waking up
void cmPowerCallback(RF_Handle h, RF_CmdHandle ch, RF_EventMask e)
{
    // turn on all the clock
    HWREG(RFC_DBELL_BASE + RFC_DBELL_O_CMDR) = CMDR_DIR_CMD_2BYTE(0x0607, 0xFFFF);
    while ((HWREG(RFC_DBELL_BASE + RFC_DBELL_O_CMDSTA)& 0x00000001) != 1);
}
#endif // RTLS_LOCATIONING_AOA

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

static RF_Object urObject;
static RF_Params urParams;

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*********************************************************************
 * @fn      urfi_initAdvCmd
 *
 * @brief   Initialize Adv RF command
 *
 * @param   None
 *
 * @return  None
 */
#if defined(FEATURE_ADVERTISER)
void urfi_initAdvCmd(void)
{
  for (uint8 i = 0; i < 3; i++)
  {
    /* Advertising channel */
    urfiAdvCmd[i].channel                = 37 + i;

    urfiAdvCmd[i].whitening.init         = 0; /* No whitening */
    urfiAdvCmd[i].pParams                = &urfiAdvParams;

    urfiAdvCmd[i].startTrigger.bEnaCmd   = 0;

  #if defined(FEATURE_SCAN_RESPONSE)
    urfiAdvCmd[i].pOutput                = &urAdvOutput;
  #else  /* FEATURE_SCAN_RESPONSE */
    urfiAdvCmd[i].pOutput                = NULL;
  #endif /* FEATURE_SCAN_RESPONSE */
  }

  /* 1st channel adv is supposed to start at a certain time */
  urfiAdvCmd[0].startTrigger.triggerType = TRIG_ABSTIME;
  /* 2nd and 3rd channel adv's are supposed to start as soon as
     the previous channel operation ends */
  urfiAdvCmd[1].startTrigger.triggerType =
  urfiAdvCmd[2].startTrigger.triggerType = TRIG_NOW;

  urfiAdvCmd[1].startTime                =
  urfiAdvCmd[2].startTime                = 0;

  urfiAdvCmd[0].condition.rule           =
  urfiAdvCmd[1].condition.rule           = COND_STOP_ON_FALSE;
  urfiAdvCmd[2].condition.rule           = COND_NEVER;

#ifdef RF_MULTIMODE
  if (RF_TIME_RELAXED == rfTimeCrit)
  {
    urfiAdvCmd[0].startTrigger.pastTrig    =
    urfiAdvCmd[1].startTrigger.pastTrig    =
    urfiAdvCmd[2].startTrigger.pastTrig    = 1;
  }
  else
  {
    urfiAdvCmd[0].startTrigger.pastTrig    =
    urfiAdvCmd[1].startTrigger.pastTrig    =
    urfiAdvCmd[2].startTrigger.pastTrig    = 0;
  }
#endif /* RF_MULTIMODE */

  urfiAdvCmd[0].pNextOp                  = (rfc_radioOp_t*) &urfiAdvCmd[1];
  urfiAdvCmd[1].pNextOp                  = (rfc_radioOp_t*) &urfiAdvCmd[2];
  urfiAdvCmd[2].pNextOp                  = NULL;
}
#endif /* FEATURE_ADVERTISER */

/*********************************************************************
 * @fn      urfi_initScanCmd
 *
 * @brief   Initialize Scan RF command
 *
 * @param   None
 *
 * @return  None
 */
#if defined(FEATURE_SCANNER)
void urfi_initScanCmd(void)
{
  urfiScanCmd.commandNo = CMD_BLE_SCANNER;
  urfiScanCmd.status = IDLE;
  urfiScanCmd.pNextOp = NULL;
  urfiScanCmd.startTime = 0;
  urfiScanCmd.startTrigger.triggerType = TRIG_NOW;
  urfiScanCmd.startTrigger.bEnaCmd = 0;
  urfiScanCmd.startTrigger.triggerNo = 0;
  
  /* uGAP controls the scan timing. Scanning late should be allowed. */
  urfiScanCmd.startTrigger.pastTrig = 1;

  urfiScanCmd.condition.rule = COND_NEVER;
  urfiScanCmd.condition.nSkip = 0;
  
  urfiScanCmd.channel = 37;
  urfiScanCmd.whitening.init = 0;
  urfiScanCmd.whitening.bOverride = 0;
  urfiScanCmd.pParams = &urfiScanParams;
  urfiScanCmd.pOutput = &pScanOutput;
  
  urfiScanParams.pRxQ = (dataQueue_t *)ull_setupScanDataEntryQueue();
}
#endif /* FEATURE_SCANNER */

#if defined(FEATURE_MONITOR)
/*********************************************************************
 * @fn      urfi_initGenericRxCmd
 *
 * @brief   Initialize Scan RF command
 *
 * @param   None
 *
 * @return  None
 */
void urfi_initGenericRxCmd(void)
{
  urfiGenericRxCmd.commandNo = CMD_BLE_GENERIC_RX;
  urfiGenericRxCmd.status = IDLE;
  urfiGenericRxCmd.pNextOp = NULL;
  urfiGenericRxCmd.startTime = 0;
  urfiGenericRxCmd.startTrigger.triggerType = TRIG_ABSTIME;
  urfiGenericRxCmd.startTrigger.bEnaCmd = 0;
  urfiGenericRxCmd.startTrigger.triggerNo = 0;
  
#ifdef RF_MULTIMODE
  urfiGenericRxCmd.startTrigger.pastTrig = (RF_TIME_RELAXED == rfTimeCrit) ? 1 : 0;
#endif /* RF_MULTIMODE */

  urfiGenericRxCmd.condition.rule = COND_NEVER;
  urfiGenericRxCmd.condition.nSkip = 0;
  
  urfiGenericRxCmd.channel = 37;
  urfiGenericRxCmd.whitening.init = 0;
  urfiGenericRxCmd.whitening.bOverride = 0;

  urfiGenericRxCmd.pParams = &urfiGenericRxParams;
  urfiGenericRxCmd.pOutput = &pGenericRxOutput;
  
  urfiGenericRxParams.pRxQ = (dataQueue_t *)ull_setupMonitorDataEntryQueue();
}
#endif /* FEATURE_MONITOR */

/*********************************************************************
 * @fn      urfi_clientEventCb
 *
 * @brief   Callback function to be invoked by RF driver
 *
 * @param   rfHandle - RF client handle
 *
 * @param   cmdHandle - RF command handle
 *
 * @param   events - RF client events
 * @param   arg - reserved for future use
 *
 * @return  none
 */
void urfi_clientEventCb(RF_Handle h, RF_ClientEvent events, void* arg)
{
  port_key_t key;

  key = port_enterCS_SW();

  if (events & RF_ClientEventRadioFree)
  {
#if defined(FEATURE_ADVERTISER)
    uble_buildAndPostEvt(UBLE_EVTDST_LL, ULL_EVT_ADV_TX_RADIO_AVAILABLE, NULL, 0);
#endif /* FEATURE_ADVERTISER */
#if defined(FEATURE_SCANNER)
    uble_buildAndPostEvt(UBLE_EVTDST_LL, ULL_EVT_SCAN_RX_RADIO_AVAILABLE, NULL, 0);
#endif /* FEATURE_SCANNER */
#if defined(FEATURE_MONITOR)
    uble_buildAndPostEvt(UBLE_EVTDST_LL, ULL_EVT_MONITOR_RX_RADIO_AVAILABLE, NULL, 0);
#endif /* FEATURE_MONITOR */
  }

  if (events & RF_ClientEventSwitchClientEntered)
  {
    if (ubleAntSwitchSel != NULL)
    {
      ubleAntSwitchSel();
    }
  }
  port_exitCS_SW(key);
}

/*********************************************************************
 * @fn      urfi_init
 *
 * @brief   Initialize radio interface and radio commands
 *
 * @param   None
 *
 * @return  SUCCESS - RF driver has been successfully opened
 *          FAILURE - Failed to open RF driver
 */
bStatus_t urfi_init(void)
{
  if (urfiHandle == NULL)
  {
    RF_Params_init(&urParams); /* Get default values from RF driver */
    urParams.nInactivityTimeout = 0; /* Do not use the default value for this */
    urParams.pClientEventCb = urfi_clientEventCb;

#ifdef RTLS_LOCATIONING_AOA
    urParams.pPowerCb = cmPowerCallback;
#endif

    /* 
     * Setup radio setup command. 
     */
    
    /* Differential mode */
    urSetup.common.config.frontEndMode  = ubFeModeBias & 0x07;
    /* Internal bias */
    urSetup.common.config.biasMode      = (ubFeModeBias & 0x08) >> 3;
    /* Keep analog configuration */
    urSetup.common.config.analogCfgMode = 0x2D;
    /* Power up frequency synth */
    urSetup.common.config.bNoFsPowerUp  =    0;
    /* 0 dBm */
    urSetup.common.txPower            = urfi_getTxPowerVal(UBLE_PARAM_DFLT_TXPOWER);
    /* Register Overrides */
    urSetup.common.pRegOverride       = (uint32_t*) ubRfRegOverride;

    /* Request access to the radio */
    urfiHandle = RF_open(&urObject, (RF_Mode*) &ubRfMode, &urSetup, &urParams);

#ifdef RTLS_LOCATIONING_AOA
    uint32_t temp = 0;
    
    RF_control(urfiHandle, RF_CTRL_SET_POWER_MGMT, &temp);
#endif

    if (urfiHandle == NULL)
    {
      return FAILURE;
    }
  }

#if defined(FEATURE_ADVERTISER)
  urfi_initAdvCmd();
#endif  /* FEATURE_ADVERTISER */

#if defined(FEATURE_SCANNER)
  urfi_initScanCmd();
#endif  /* FEATURE_SCANNER */
  
#if defined(FEATURE_MONITOR)
  urfi_initGenericRxCmd();
#endif  /* FEATURE_SCANNER */
  
  return SUCCESS;
}

/*********************************************************************
 * CALLBACKS
 */

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

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
uint16 urfi_getTxPowerVal(int8 txPower)
{
  uint8 i;

  for (i = 0; i < ubTxPowerTable.numTxPowerVal; i++)
  {
    if (ubTxPowerTable.pTxPowerVals[i].dBm == txPower)
    {
      return ubTxPowerTable.pTxPowerVals[i].txPowerVal;
    }
  }

  return UBLE_TX_POWER_INVALID;
}


/*********************************************************************
*********************************************************************/
