/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x AND AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : usb_hal.h
//
// Project     : TUSB926x Firmware.
//
// Description : Header file for USB HAL.
//
//   (C) Copyright 2009 by Texas Instruments Incorporated.
//   All rights reserved.
//
// Revision History:
//   MM/DD/YY
//   01/15/09 - Brian Quach - Creation.
//
//=======================================================================================

/*! @file
 *
 * Header file for the USB hardware abstraction layer.
 *
 */

#include "reg_io.h"
#include "tusb9260.h"
#include "tusb9260_types.h"

#ifndef _USB_HAL_H_
#define _USB_HAL_H_

/*----------------------------------------------------------------------------+
| Macros                                                                      |
+----------------------------------------------------------------------------*/
#ifndef ENABLE_U1_U2_TRANSITIONS
#define ENABLE_U1_U2_TRANSITIONS  0  // (Default = 0) Set to 1 to enable host & device initiated transitions to U1/U2 states. Recommended to leave this disabled for maximum performance and compatibility.
#endif

#define DISABLE_U1_U2_INITIATE_WHILE_XFER_ACTIVE  1  // (Default = 1) Set to 1 to disable device-initiated U1/U2 while there is transfer activity. Fixes low throughput issue with Fresco and Intel hosts.

#define ENABLE_USB_SSC            0   // (Default = 0) Set to 1 to enable spread spectrum clocking. (Disabled by default for maximum compatibility with various host controllers which may have non-compliance SSC if they derive clock from motherboard)
#define ENABLE_USB_PHY_SUSPEND    1   // (Default = 1) Set to 0 to disable PHY suspend modes and USB core clock gating.
#define DISABLE_SCRAMBLING        0   // (Default = 0) Set to 1 to disable scrambling.

#define FORCE_USB20_ONLY    0   // (Default = 0) Set to 1 to force USB 2.0 only mode.

/* USB 3.0 parameters. */
#define MAX_BURST_SIZE  0x0F  // 0 represents 1 burst.
#define NUM_P           3     // # of Rx buffers to be reported in ACK TP. (zero-based, must be less than or equal to max burst size).  Value of 3 shows best performance in 4k Write testing.

/* Endpoint numbers */
#define EP0 0x00
#define EP1 0x01
#define EP2 0x02
#define EP3 0x03

/* Endpoint direction masks */
#define ENDPT_DIRECTION_MASK        0x80   // 0 for OUT, 0x80 for IN
#define ENDPT_DIRECTION_BIT_OFFSET  7

typedef enum
{
    ENDPT_DIRECTION_OUT = 0x00,
    ENDPT_DIRECTION_IN  = 0x80,
} ENDPT_DIR_T;

#define SETUP_PACKET_DATA_LENGTH  8 /* bytes */

/* Interrupt/event numbers */
#define EVNT_BUFF_0      0
#define EP_INTR_NUM      EVNT_BUFF_0
#define NON_EP_INTR_NUM  EVNT_BUFF_0

/*----------------------------------------------------------------------------+
| USB Controller Registers and Bits                                           |
+----------------------------------------------------------------------------*/

#define USB_BAR   0xFC00C000
#define USB_REG_OFF(reg_off) (reg_off + USB_BAR)

// Convert logical endpt # to physical HW endpt #. (multiply the endpt number by 2 and add 1 if IN direction)
#define LEP2PEP(bEndptNum)  ((((bEndptNum) & ~ENDPT_DIRECTION_MASK) << 1) | ((bEndptNum) >> ENDPT_DIRECTION_BIT_OFFSET))

/**************************************************************/
/* USB Register offsets (must be used with BAR offset macros) */
/**************************************************************/

/* Global Registers */
#define GCTL_REG_OFF        0x110
#define GEVTEN_REG_OFF      0x114
#define GSTS_REG_OFF        0x118   /* Global status register */
#define GSNPSID_REG_OFF     0x120   /* Core ID and release number */
#define GBUSERRADDR_REG_OFF 0x130

#define GDBGFIFOSPACE_REG_OFF           0x160
#define GDBGFIFOSPACE_AVAIL_OFFSET      16
#define GDBGLTSSM_REG_OFF               0x164
#define GDBGLTSSM_LINK_STATE_MASK       0x03C00000
#define GDBGLTSSM_LINK_STATE_OFFSET     22
#define GDBGLTSSM_LINK_SUBSTATE_MASK    0x003C0000
#define GDBGLTSSM_LINK_SUBSTATE_OFFSET  18

#define GUSB2PHYCFG_REG_OFF       0x200
#define PHYCFG_SUSPEND_ENABLE_BIT 0x00000040
#define PHYCFG_SOFT_RESET_BIT     0x80000000

#define GUSB3PIPECTL_REG_OFF                0x2C0
#define PIPECTL_P3_EXIT_SIGNAL_TO_P2_BIT    0x00000400
#define PIPECTL_SUSPEND_SS_PHY_ENABLE_BIT   0x00020000
#define PIPECTL_P0P1_RX_VALID_LOW_BIT       0x00040000
#define PIPECTL_PHY_SOFT_RESET_BIT          0x80000000


#define GTXFIFOSIZ0_REG_OFF         0x300  /* do not use directly. use macro below */
#define GTXFIFOSIZ(bFifoNum)        USB_REG_OFF(GTXFIFOSIZ0_REG_OFF + (0x04 * bFifoNum))
#define GTXFIFOSIZ_FIFO_DEPTH_MASK  0xFFFF

#define GEVNTADR0_LO_REG_OFF        0x400  /* do not use directly. use macro below */
#define GEVNTADR_LO(bEvtBuffNum)    USB_REG_OFF(GEVNTADR0_LO_REG_OFF + (0x10 * bEvtBuffNum))
#define GEVNTADR0_HI_REG_OFF        0x404  /* do not use directly. use macro below */
#define GEVNTADR_HI(bEvtBuffNum)    USB_REG_OFF(GEVNTADR0_HI_REG_OFF + (0x10 * bEvtBuffNum))

#define GEVNTSIZ0_REG_OFF           0x408  /* do not use directly. use macro below */
#define GEVNTSIZ(bEvtBuffNum)       USB_REG_OFF(GEVNTSIZ0_REG_OFF + (0x10 * bEvtBuffNum))
#define GEVNTSIZ0_BUFFER_SIZE_MASK  0x0000FFFF

#define GEVNTCOUNT0_REG_OFF         0x40C  /* do not use directly. use macro below */
#define GEVNTCOUNT(bEvtBuffNum)     USB_REG_OFF(GEVNTCOUNT0_REG_OFF + (0x10 * bEvtBuffNum))
#define EVNT_CNT_MASK               0x0000FFFF


/* Device Common Registers */
#define DCFG_REG_OFF        0x700   /* Device config */
#define DCTL_REG_OFF        0x704   /* Device control */
#define DEVTEN_REG_OFF      0x708   /* Event enable */
#define DSTS_REG_OFF        0x70C   /* Device status */
#define DGCMDPAR_REG_OFF    0x710   /* Generic command parameter */
#define DGCMD_REG_OFF       0x714   /* Generic command */
#define DALEPENA_REG_OFF    0x720   /* Logical Endpt Enable - Even/Odd : OUT/IN */


/* Endpt Command Regs */
#define DEPCMDPAR2_0_REG_OFF  0x800  /* do not use directly. use macro below */
#define DEPCMDPAR2(bEndptNum) USB_REG(DEPCMDPAR2_0_REG_OFF + (0x10 * LEP2PEP(bEndptNum))))
#define DEPCMDPAR1_0_REG_OFF  0x804  /* do not use directly. use macro below */
#define DEPCMDPAR1(bEndptNum) USB_REG_OFF(DEPCMDPAR1_0_REG_OFF + (0x10 * LEP2PEP(bEndptNum)))
#define DEPCMDPAR0_0_REG_OFF  0x808  /* do not use directly. use macro below */
#define DEPCMDPAR0(bEndptNum) USB_REG_OFF(DEPCMDPAR0_0_REG_OFF + (0x10 * LEP2PEP(bEndptNum)))
#define DEPCMD0_REG_OFF   0x80C  /* do not use directly. use macro below */
#define DEPCMD(bEndptNum) USB_REG_OFF(DEPCMD0_REG_OFF + (0x10 * LEP2PEP(bEndptNum)))


/* GCTL Bits */
#define GCTL_U2_RESET_ECN_BIT          0x00010000  // Typo in databook lists this as bit 18, but it should be bit 16.
#define GCTL_CORE_SOFT_RESET           0x00000800
#define GCTL_DISABLE_SCRAMBLING        0x00000008
#define GCTL_DISABLE_CLOCK_GATING_BIT  0x00000001

/* DEVTEN Bits */
#define DEVTEN_RESUME_WKUP_EVNT   0x00000010
#define DEVTEN_LINK_STATE_CHANGE  0x00000008
#define DEVTEN_CONNECT_DONE       0x00000004
#define DEVTEN_USB_RESET          0x00000002
#define DEVTEN_DISCONNECT         0x00000001

/* DCTL Bits */
#define DCTL_RUN_BIT                  0x80000000
#define DCTL_SOFT_RESET_BIT           0x40000000
#define DCTL_TARGET_UL_STATE_MASK     0x001E0000
#define DCTL_TARGET_UL_STATE_OFFSET   17
#define DCTL_TEST_MODE_MASK           0x0000001E
#define DCTL_TEST_MODE_OFFSET         1
#define DCTL_U2_INITIATE_ENABLE_BIT   0x00001000
#define DCTL_U2_ACCEPT_ENABLE         0x00000800
#define DCTL_U1_INITIATE_ENABLE_BIT   0x00000400
#define DCTL_U1_ACCEPT_ENABLE         0x00000200
#define DCTL_UL_STATE_CHNG_REQ_MASK   0x000001E0
#define DCTL_UL_STATE_CHNG_REQ_OFFSET 5

#define DCTL_REMOTE_WAKEUP_REQ        0x8

/* DGCMD Bits */
#define DGCMD_CMD_ACT_BIT           0x00000400
#define DGCMD_CMD_IOC_BIT           0x00000100

/* DGCMD command types */
#define DGCMD_TYPE_SET_SEL_PARAM    0x00000002
#define DGCMD_TYPE_WAKE_DEVICE      0x00000003
#define DGCMD_TYPE_LTM              0x00000004
#define DGCMD_TYPE_SET_EP_NRDY      0x0000000C

/* DALEPENA Bits */
#define DALEPENA_EP0_OUT      0x00000001
#define DALEPENA_EP0_IN       0x00000002

/* DCFG Bits */
#define DCFG_IGNORE_PP_FLAG   0x00800000
#define DCFG_LPM_CAPABLE_BIT  0x00400000
#define DCFG_NUM_P_MASK       0x003E0000
#define DCFG_NUM_P_OFFSET     17
#define DCFG_INTR_NUM_MASK    0x0001F000
#define DCFG_INTR_NUM_OFFSET  12
#define DCFG_DEV_ADDR_MASK    0x000003F8
#define DCFG_DEV_ADDR_OFFSET  3

/* DSTS Bits */
#define DSTS_RX_FIFO_EMPTY_BIT              0x00020000
#define DSTS_SPEED_MASK                     0x00000007
#define DSTS_USB_LINK_STATE_MASK            0x003C0000
#define DSTS_USB_LINK_STATE_OFFSET          18
#define DSTS_DEV_SPEED_MASK                 0x00000007
#define DSTS_SPEED_HS_PHY_30MHZ_OR_60MHZ    0x0
#define DSTS_SPEED_FS_PHY_30MHZ_OR_60MHZ    0x1
#define DSTS_SPEED_LS_PHY_6MHZ              0x2
#define DSTS_SPEED_FS_PHY_48MHZ             0x3
#define DSTS_SPEED_SS_PHY_125MHZ_OR_250MHZ  0x4

/* DEPCMD bits */
#define DEPCMD_HIPRI_FORCE_RM_BIT     0x00000800
#define DEPCMD_CMD_ACT_BIT            0x00000400
#define DEPCMD_CMD_IOC_BIT            0x00000100
#define DEPCMD_STREAM_ID_OFFSET       16
#define DEPCMD_XFER_RSC_MASK          0x007F0000
#define DEPCMD_XFER_RSC_INDEX_OFFSET  16
#define DEPCMD_CMD_TYPE_MASK          0x000000FF

/* DEPCMD command types */
#define DEPCMD_TYPE_SET_EP_CONFIG  0x00000001
#define DEPCMD_TYPE_SET_EP_XFER_RESOURCE_CONFIG  0x00000002
#define DEPCMD_TYPE_GET_DATA_SEQ   0x00000003
#define DEPCMD_TYPE_SET_STALL      0x00000004
#define DEPCMD_TYPE_CLEAR_STALL    0x00000005
#define DEPCMD_TYPE_START_XFER     0x00000006    /* uses PARM0 and PARM 1 for descriptor addr */
#define DEPCMD_TYPE_UPDATE_XFER    0x00000007
#define DEPCMD_TYPE_END_XFER       0x00000008
#define DEPCMD_START_NEW_CONFIG    0x00000009

/* PAR0 */
#define PAR0_EPTYPE_OFFSET       1
#define PAR0_EPTYPE_CONTROL      0x0
#define PAR0_EPTYPE_ISOCHRONOUS  0x1
#define PAR0_EPTYPE_BULK         0x2
#define PAR0_EPTYPE_INTERRUPT    0x3

#define PAR0_MPS_OFFSET           3   /* max pkt size */
#define PAR0_FIFO_NUM_MASK        0x003E0000
#define PAR0_FIFO_NUM_OFFSET      17
#define PAR0_BURST_SIZE_MASK      0x03C00000
#define PAR0_BURST_SIZE_OFFSET    22
#define PAR0_DATA_SEQ_NUM_MASK    0x7C000000
#define PAR0_DATA_SEQ_NUM_OFFSET  26

#define PAR0_NUM_XFER_RES_MASK    0x0000FFFF

/* PAR1 */
#define PAR1_EP_NUM_OFFSET    26
#define PAR1_EP_DIR_BIT       0x02000000    /* Set for IN endpts */
#define PAR1_STRM_CAP         0x01000000
#define PAR1_STREAM_EVENT_EN  0x00002000
#define PAR1_XFER_NRDY_EN     0x00000400
#define PAR1_XFER_IN_PROG_EN  0x00000200
#define PAR1_XFER_CMPLT_EN    0x00000100

/* TRB Status bits */
#define TRB_STATUS_TRBSTS_MASK       0xF8000000  /* TRB Status */
#define TRB_STATUS_BUFFER_SIZE_MASK  0x00FFFFFF  /* Buffer size in bytes. [Range: 0 bytes - 16 MB] Must be 0 for Status TRB and 8 for Setup TRB */

#define TRB_STATUS_TRBSTS_SETUP_PENDING  0x04

/* TRB Control values */
#define TRBCTRL_NORMAL           0x01
#define TRBCTRL_CONTROL_SETUP    0x02  /* buffer ptr can point to TRB address */
#define TRBCTRL_CONTROL_STATUS_2 0x03  /* for 2-stage control xfer (no data stage) */
#define TRBCTRL_CONTROL_STATUS_3 0x04  /* for 3-stage control xfer (with data stage) */
#define TRBCTRL_CONTROL_DATA     0x05
#define TRBCTRL_LINK_TRB         0x08

/* TRB Control bits */
#define TRB_CTRL_HWO_BIT         0x00000001   /* HW owner of descriptor (SW should set to 1, HW clears it unless Link TRB or Short packet on OUT endpt) */
#define TRB_CTRL_LST_BIT         0x00000002   /* Last TRB (Do not set for ISOCH endpts) */
#define TRB_CTRL_CHN_BIT         0x00000004   /* Chain buffers */
#define TRB_CTRL_CSP_BIT         0x00000008   /* Continue on Short Packet */
#define TRB_CTRL_TRBCTL_MASK     0x000003F0   /* TRB Control */
#define TRB_CTRL_TRBCTL_OFFSET   4
#define TRB_CTRL_ISP_BIT         0x00000400   /* Interrupt on Short Packet */
#define TRB_CTRL_IOC_BIT         0x00000800   /* Interrupt on Complete */
#define TRB_CTRL_STRM_ID_MASK    0x3FFFC000   /* Stream ID for USB 3.0 bulk endpts (Frame Num/Index for ISOCH endpts) */
#define TRB_CTRL_STRM_ID_OFFSET  14

/* Endpoint Event */
#define DEPEVT_EVENT_STATUS_MASK      0x0000F000
#define DEPEVT_EVENT_STATUS_OFFSET    12
#define DEPEVT_EVENT_TYPE_MASK        0x000003C0
#define DEPEVT_EVENT_TYPE_OFFSET      6
#define DEPEVT_PHYSICAL_EP_NUM_MASK   0x0000003E
#define DEPEVT_PHYSICAL_EP_NUM_OFFSET 1

/* Endpoint Event Types */
#define DEPEVT_TYPE_EP_CMD_CMPLT          0x07
#define DEPEVT_TYPE_STREAM_EVT            0x06
#define DEPEVT_TYPE_SETUP_OVERWRITE       0x05
#define DEPEVT_TYPE_FIFO_OVER_UNDER_RUN   0x04
#define DEPEVT_TYPE_XFER_NOT_READY        0x03
#define DEPEVT_TYPE_XFER_IN_PROGRESS      0x02
#define DEPEVT_TYPE_XFER_COMPLETE         0x01

/* Device-specific Event */
#define DEVT_EVENT_INFO_MASK 0x00FF0000
#define DEVT_EVENT_INFO_OFFSET 16
#define DEVT_EVENT_TYPE_MASK 0x00000F00
#define DEVT_EVENT_TYPE_OFFSET 8

/* Device-specific Event Types */
#define DEVT_U2_TIMEOUT_RCVD          0x0D
#define DEVT_VENDOR_DEV_TEST_RCVD     0x0C
#define DEVT_EVNT_OVERFLOW            0x0B
#define DEVT_CMD_CMPLT                0x0A
#define DEVT_ERRATIC_ERROR            0x09
#define DEVT_TYPE_SOF                 0x07
#define DEVT_TYPE_EOPF                0x06
#define DEVT_TYPE_RESUME              0x04  // updated for 1.01a IP.
#define DEVT_TYPE_USB_LINK_STATE_CHNG 0x03
#define DEVT_TYPE_CONNECTION_DONE     0x02
#define DEVT_TYPE_USB_RESET           0x01
#define DEVT_TYPE_DISCONNECT          0x00


/**** DSTS USB/LINK STATES *****/

#define DEV_CTRL_HALT_STATE 0x0E   // this state must be polled, no link state change event is generated.

// HS/FS/LS States
#define ON_STATE            0x00
#define SLEEP_STATE         0x02
#define SUSPEND_STATE       0x03
#define VBUS_OFF_STATE      0x04
#define EARLY_SUSPEND_STATE 0x05

// SS Link States
#define U0_STATE            0x00
#define U1_STATE            0x01
#define U2_STATE            0x02
#define U3_STATE            0x03
#define SS_DISABLED_STATE   0x04
#define RX_DETECT_STATE     0x05
#define SS_INACTIVE_STATE   0x06
#define POLLING_STATE       0x07
#define RECOVERY_STATE      0x08
#define HOT_RESET_STATE     0x09
#define COMPLIANCE_STATE    0x0A
#define LOOPBACK_STATE      0x0B

/********************************/

/*----------------------------------------------------------------------------+
| Structures                                                                  |
+----------------------------------------------------------------------------*/


typedef enum
{
    EP0_STATE_IDLE       = 0,    /* Ready to receive SETUP packet */
    EP0_STATE_DATA_IN    = 1,
    EP0_STATE_DATA_OUT   = 2,
    EP0_STATE_STATUS_IN  = 3,
    EP0_STATE_STATUS_OUT = 4,
    EP0_STATE_ERROR      = 5    /* Fatal error. IN and OUT should be STALLED so we will get reset by host. */
} eEP0_STATE_T;

typedef enum
{
    USB_PM_STATE_RESUME,
    USB_PM_STATE_RESET,
    USB_PM_STATE_WAKE_UP
} eUSB_PM_STATE_T;


typedef enum
{
    USB_LOW_SPEED     = 0,
    USB_FULL_SPEED    = 1,
    USB_HIGH_SPEED    = 2,
    USB_SUPER_SPEED   = 3,
    USB_SPEED_UNKNOWN = 4
} eUSB_DEVICE_SPEED_T;


typedef enum
{
    USB_DEVICE_STATE_NOT_CONNECTED,
    USB_DEVICE_STATE_DEFAULT,     /* Device has been reset */
    USB_DEVICE_STATE_ADDRESSED,   /* Set address request processed */
    USB_DEVICE_STATE_CONFIGURED   /* Set config request processed */
} eUSB_DEVICE_STATE_T;

//typedef enum
//{
//    USB_DEVICE_POWER_BUS,
//    USB_DEVICE_POWER_SELF,
//    USB_DEVICE_POWER_BATTERY
//} eUSB_DEVICE_POWER_T;

typedef enum
{
    USB_PM_SUSPEND,
    USB_PM_RESUME,
    USB_PM_RESET,
    USB_PM_DISCONNECT
} eUSB_DEVICE_PM_STATE_T;

typedef enum
{
    USB_MSC_BOT,
    USB_MSC_UAS
} eUSB_MS_CLASS_T;



typedef struct _EP_INFO_T
{
    void       *pBuffer;         /* ptr to buffer for Tx or Rx */
    UINT32_T   dByteCount;       /* total number of bytes transfered */
    INT32_T    dBytesRemaining;  /* total length of data to be transfered */
    UINT32_T   dXferLength;      /* xfer length specified in the TRB before starting xfer */

    BOOLEAN_T  bStalled;
    UINT16_T   wMaxPktSize;

    UINT16_T   wStreamID;

    TRANSFER_REQUEST_BLOCK_T  *pTRB;
    UINT32_T   dXferRscIndex;
    BOOLEAN_T  bXferActive;

    BOOLEAN_T  wrap_window_xfer;

} EP_INFO_T;


typedef struct _USB_SETUP_PACKET_T
{
    UINT8_T    bmRequestType;
    UINT8_T    bRequest;
    UINT16_T   wValue;
    UINT16_T   wIndex;
    UINT16_T   wLength;
} USB_SETUP_PACKET_T;


typedef struct _TRB_RING_INFO_T
{
    UINT32_T     bytes_remaining;  /* indicates the amount of data not yet setup in the TRB ring */
    UINT32_T     xfer_length[TRB_RING_IN_SIZE - 1];
    UINT32_T     index;          /* index of next TRB that needs to be setup */
    UINT32_T     index_pending;  /* index of next TRB that should be completed */
    UINT8_T      *buffer_ptr;    /* ptr to data buffer for next TRB that needs to be setup */
    UINT32_T     stream_id;      /* stream ID to be used in TRBs */
} TRB_RING_INFO_T;


typedef struct _USB_DEVICE_T
{
    UINT32_T               usb_core_version;

    eUSB_DEVICE_SPEED_T    dev_speed;
    eUSB_DEVICE_STATE_T    dev_state;
//    eUSB_DEVICE_POWER_T    dev_power_mode;
    eUSB_DEVICE_PM_STATE_T dev_pm_state;

    eEP0_STATE_T           ep0_state;
    BOOLEAN_T              ep0_three_stage_xfer;
    BOOLEAN_T              ep0_zlp_pending;  /* indicates whether we need to Tx or Rx Zero Length Packet to end data stage. */

    BOOLEAN_T              bConfigEP0SetupXfer;  /* needs to be done once after power-on */
    BOOLEAN_T              bUSBResetInitComplete;

    UINT32_T               dMaxEventCnt;

    UINT32_T               dDisconnectCountDown; /* to delay HDD spin-down */
    UINT32_T               dSuspendCountDown;    /* to debounce suspend and delay HDD spin-down (USB 2.0 suspend link state occurs during disconnect) */

    eUSB_MS_CLASS_T        active_mass_storage_class;

    BOOLEAN_T              bBOT_PersistentStall;  /* for BOT usage */

    BOOLEAN_T              bFunctionWakeDeviceNotificationPending;
    UINT8_T                bFunctionWakeInterfaceNum;

    UINT8_T                bSerialNumStringDescIndex;

    BOOLEAN_T              bSelfPoweredCapable;
    BOOLEAN_T              bLowPowerSuspendState[MAX_INTERFACE_NUM + 1];
    BOOLEAN_T              bRemoteWakeupCapable;
    BOOLEAN_T              bRemoteWakeupEnabled[MAX_INTERFACE_NUM + 1];
    UINT16_T               wTestMode;
    UINT8_T                bCurrentConfigNum;

    UINT8_T                bCurrentInterfaceAltSetting[MAX_INTERFACE_NUM + 1];

    BOOLEAN_T              bIsU1Enabled;
    BOOLEAN_T              bIsU2Enabled;
    BOOLEAN_T              bIsLTMEnabled;

    USB_SETUP_PACKET_T     setup_packet;        /* extract setup pkt info before processing request */

    EP_INFO_T              ep_info_OUT[MAX_EP_NUM + 1];     /* endpoint info for OUT direction */
    EP_INFO_T              ep_info_IN[MAX_EP_NUM + 1];      /* endpoint info for IN direction */

    TRB_RING_INFO_T        trb_ring_info_IN;
    TRB_RING_INFO_T        trb_ring_info_OUT;

    UINT32_T               *event_ptr[NUM_EVNT_BUFFS];  /* ptr to current event */
    UINT32_T               *event_buff[NUM_EVNT_BUFFS]; /* ptr to event buffer */
    UINT32_T               *event_buff_end[NUM_EVNT_BUFFS];  /* ptr to end of event buffer */
} USB_DEVICE_T;


/*----------------------------------------------------------------------------+
| Externs                                                                     |
+----------------------------------------------------------------------------*/

extern USB_DEVICE_T usb_dev;

extern void (*pUsbStackSetupPktCallback)(void);
extern void (*pUsbStackDataXferCallback)(UINT32_T ep_num, EP_INFO_T *ep_info);
extern void (*pUsbStackPowerMngmtCallback)(eUSB_DEVICE_PM_STATE_T pm_state);


/*----------------------------------------------------------------------------+
| Function Prototypes                                                         |
+----------------------------------------------------------------------------*/

void usb_hal_set_endpt_stall(UINT32_T ep_num);
void usb_hal_clear_endpt_stall(UINT32_T ep_num);
BOOLEAN_T usb_hal_is_endpt_stalled(UINT32_T ep_num);

void usb_hal_set_address(UINT32_T address);

void usb_hal_handle_usb_reset_stage2(void);

void usb_hal_ep0_setup_stage(void);
void usb_hal_ep0_status_stage(ENDPT_DIR_T direction);
STATUS_T usb_hal_ep0_io_continue(ENDPT_DIR_T direction, void *buffer, UINT32_T byte_cnt);
STATUS_T usb_hal_ep0_io_request(ENDPT_DIR_T direction, void *buffer, UINT32_T byte_cnt);

void usb_hal_config_ep0(UINT32_T tx_fifo, BOOLEAN_T config_xfer_resources);
void usb_hal_configure_endpts(void);
void usb_hal_set_test_mode(UINT32_T test_mode);

void usb_hal_set_U1_initiate_enable(BOOLEAN_T enable);
void usb_hal_set_U2_initiate_enable(BOOLEAN_T enable);
void usb_hal_set_sel(UINT32_T u2pel);

STATUS_T usb_hal_io_request(UINT32_T ep_num, void *pBuffer, UINT32_T byte_cnt, UINT32_T stream_id);
STATUS_T usb_hal_cancel_io_request(UINT32_T ep_num);
void usb_hal_cancel_all_io_requests(void);
STATUS_T usb_hal_config_next_trb(UINT32_T ep_num);

eUSB_DEVICE_SPEED_T usb_hal_get_connect_speed();
STATUS_T usb_hal_dep_get_data_seq(UINT32_T ep_num, UINT32_T *data_seq_num);
void usb_hal_remote_wakeup(void);
void usb_hal_function_wake_device(UINT32_T interface_num);

STATUS_T usb_hal_update_transfer(UINT32_T ep_num);

void usb_hal_connect(void);
void usb_hal_disconnect(void);

void usb_hal_init(void (*pSetupPktCallback)(void), void (*pDataXferCallback)(UINT32_T ep_num, EP_INFO_T *ep_info),
                  void (*pPowerMngmtCallback)(eUSB_DEVICE_PM_STATE_T pm_state));

void usb_hal_isr(void);

#if DEBUG_LEVEL >= 1
void dump_usb_core_debug(void);
#endif

/*----------------------------------------------------------------------------+
| Inline Functions                                                            |
+----------------------------------------------------------------------------*/

/*****************************************************************************
 * Function: usb_hal_get_ep_info_ptr
 *************************************************************************//**
 * This function retrieves the pointer to the endpoint information
 * structure.
 *
 * @param[in] ep_num endpoint number. Bit 7 indicates the direction (0=OUT, 1=IN).
 * @param[out] ep_info pointer to memory to store pointer to endpoint information structure.
 *
 * @retval None.
 *
 ******************************************************************************
 */

inline void usb_hal_get_ep_info_ptr(UINT8_T ep_num, EP_INFO_T **ep_info)
{
    if ((ep_num & ENDPT_DIRECTION_MASK) == ENDPT_DIRECTION_OUT)
    {
        *ep_info = &usb_dev.ep_info_OUT[ep_num & ~ENDPT_DIRECTION_MASK];
    }
    else
    {
        *ep_info = &usb_dev.ep_info_IN[ep_num & ~ENDPT_DIRECTION_MASK];
    }

    return;
}

/*****************************************************************************
 * Function: usb_hal_get_usb_link_state
 *************************************************************************//**
 * This function returns the USB link state.
 *
 * @param None.
 *
 * @return The USB link state value. (meaning depends on the current connection speed)
 * - USB 3.0 Link States:
 *   - 0x00 U0_STATE
 *   - 0x01 U1_STATE
 *   - 0x02 U2_STATE
 *   - 0x03 U3_STATE
 *   - 0x04 SS_DISABLED_STATE
 *   - 0x05 RX_DETECT_STATE
 *   - 0x06 SS_INACTIVE_STATE
 *   - 0x07 POLLING_STATE
 *   - 0x08 RECOVERY_STATE
 *   - 0x09 HOT_RESET_STATE
 *   - 0x0A COMPLIANCE_STATE
 *   - 0x0B LOOPBACK_STATE
 * - USB 2.0 Link States:
 *   - 0x00 ON_STATE
 *   - 0x02 SLEEP_STATE
 *   - 0x03 SUSPEND_STATE
 *   - 0x04 DEV_CTRL_HALT_STATE
 *   - 0x05 EARLY_SUSPEND_STATE
 *
 ******************************************************************************
 */

inline UINT32_T usb_hal_get_usb_link_state(void)
{
    return((READ32(USB_REG_OFF(DSTS_REG_OFF)) & DSTS_USB_LINK_STATE_MASK) >> DSTS_USB_LINK_STATE_OFFSET);
}

#endif
