/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x
 */

//=======================================================================================
// Filename    : ahci.h
//
// Project     : TUSB926x Firmware.
//
// Description : Header file for ACHI SATA controller driver.
//
//   (C) Copyright 2009 by Texas Instruments Incorporated.
//   All rights reserved.
//
// Revision History:
//   MM/DD/YY
//   03/04/09 - Brian Quach - Creation.
//
//=======================================================================================

/*! @file
 *
 * Header file for the Serial ATA Advanced Host Controller Interface driver.
 *
 */

#ifndef _AHCI_H_
#define _AHCI_H_

#include "scsi.h"
#include "tusb9260.h"
#include "tusb9260_types.h"

/*----------------------------------------------------------------------------+
| Macros                                                                      |
+----------------------------------------------------------------------------*/

/** (Default = 1) When enabled, 4KB logical sectors are emulated
 *  for HDDs larger than 2.2 TB so they can be supported under
 *  WinXP. This optional also automatically aligns 4KB physical
 *  sectors. */
#define ENABLE_LARGE_SECTOR_EMULATION 0

/** (Default = 0) When enabled, the SATA drive is put into
 *  standby power mode (spins down HDD) when USB is disconnected
 *  or suspended. Enabling this option may cause issues upon USB
 *  attach or resume after active sleep/hibernate because the
 *  USB core will be held busy with datapath RAM access during
 *  the first R/W command while the drive is spinning up. The
 *  device will not be able to respond to any USB requests (e.g.
 *  EP0 requests) until the datapath RAM access is complete. As
 *  a result, the host may issue a USB bus reset and
 *  re-enumerate the device. */
#define ENABLE_SATA_STANDBY_POWER_MODE 0

/** (Default = 0) When Link power management is enabled, hot
 *  plug operation fails because device removal cannot be
 *  detected. Device throughput is also reduced. */
#define AHCI_LINK_POWER_MGMT_ENABLE 0

/** (Default = 0) Set to 1 to make the TUSB926x appear as a
 *  removable media drive */
#ifndef REMOVABLE_MEDIA_DEVICE
#define REMOVABLE_MEDIA_DEVICE 0
#endif

/** (Default = 0) Set to 1 to force SATA speed to Gen-1 */
#define FORCE_GEN1_SPEED 0

#define NUM_AHCI_PORTS 1

#define ATA_CALLBACK_QUEUE_DEPTH (AHCI_NCQ_DEPTH * 3)

/*----------------------------------------------------------------------------+
| SATA Controller Registers and Bits                                           |
+----------------------------------------------------------------------------*/
#define AHCI_BAR 0xFB000000
#define AHCI_REG_OFF(reg_off) (reg_off + AHCI_BAR)

#define CAP_REG_OFF 0x000
#define CAP_NCS_MASK 0x00001F00

/* Global HBA Control Register */
#define GHC_REG_OFF 0x004
#define GHC_INT_EN_BIT 0x00000002
#define GHC_HBA_RESET_BIT 0x00000001

/* Interrupt Status Register */
#define IS_REG_OFF 0x008

/* Ports Implemented Register */
#define PI_REG_OFF 0x00C

#define PI_PORT0 0x00000001
#define PI_PORT1 0x00000002
#define PI_PORT2 0x00000004
#define PI_PORT3 0x00000008
#define PI_PORT4 0x00000010
#define PI_PORT5 0x00000020
#define PI_PORT6 0x00000040
#define PI_PORT7 0x00000080

/* AHCI Version Register */
#define VS_REG_OFF 0x010

/* BIST Activate FIS Register */
#define BISTAFR_REG_OFF 0x0A0

/* BIST Control Register */
#define BISTCR_REG_OFF 0x0A4

/* Diagnostic Registers */
#define DIAGNR_REG_OFF 0x0B4
#define DIAGNR1_REG_OFF 0x0B8
#define DIAGNR1_IGNORE_I_BIT_ENABLE_BIT 0x00010000 /* bit 16 */

/* Port Registers */
#define PxCLB(bPortNum) AHCI_REG_OFF(0x100 + (bPortNum * 0x80))
#define PxCLBU(bPortNum) AHCI_REG_OFF(0x104 + (bPortNum * 0x80))
#define PxFB(bPortNum) AHCI_REG_OFF(0x108 + (bPortNum * 0x80))
#define PxIS(bPortNum) AHCI_REG_OFF(0x110 + (bPortNum * 0x80))
#define PxIE(bPortNum) AHCI_REG_OFF(0x114 + (bPortNum * 0x80))
#define PxCMD(bPortNum) AHCI_REG_OFF(0x118 + (bPortNum * 0x80))
#define PxTFD(bPortNum) AHCI_REG_OFF(0x120 + (bPortNum * 0x80))
#define PxSIG(bPortNum) AHCI_REG_OFF(0x124 + (bPortNum * 0x80))
#define PxSSTS(bPortNum) AHCI_REG_OFF(0x128 + (bPortNum * 0x80))
#define PxSCTL(bPortNum) AHCI_REG_OFF(0x12C + (bPortNum * 0x80))
#define PxSERR(bPortNum) AHCI_REG_OFF(0x130 + (bPortNum * 0x80))
#define PxSACT(bPortNum) AHCI_REG_OFF(0x134 + (bPortNum * 0x80))
#define PxCI(bPortNum) AHCI_REG_OFF(0x138 + (bPortNum * 0x80))
#define PxSNTF(bPortNum) AHCI_REG_OFF(0x13C + (bPortNum * 0x80))
#define PxDMACR(bPortNum) AHCI_REG_OFF(0x170 + (bPortNum * 0x80))
#define PxPHYCTRL(bPortNum) AHCI_REG_OFF(0x178 + (bPortNum * 0x80)) /* Always use MODIFY32() to change values for this register */
#define PxPHYSTAT(bPortNum) AHCI_REG_OFF(0x17C + (bPortNum * 0x80))

#define PCMD_ST_BIT 0x00000001
#define PCMD_FRE_BIT 0x00000010
#define PCMD_FR_BIT 0x00004000
#define PCMD_CR_BIT 0x00008000
#define PCMD_CPS_BIT 0x00010000
#define PCMD_CPS_OFFSET 16
#define PCMD_HPCP_BIT 0x00040000
#define PCMD_CPD_BIT 0x00100000
#define PCMD_ASP_BIT 0x08000000
#define PCMD_ALPE_BIT 0x04000000
#define PCMD_ATAPI_BIT 0x01000000
#define PCMD_APSTE_BIT 0x00800000
#define PCMD_CCS_MASK 0x00001F00
#define PCMD_CCS_OFFSET 8
#define PCMD_ICC_MASK 0xF0000000
#define PCMD_ICC_OFFSET 28

#define PSSTS_DET_MASK 0x0000000F
#define PSSTS_DET_NO_DEVICE 0x00000000
#define PSSTS_DET_DEV_DETECT 0x00000001
#define PSSTS_DET_PHY_READY 0x00000003
#define PSSTS_DET_PHY_OFFLINE 0x00000004 /* indicates disabled interface or BIST running */
#define PSSTS_DET_BIT0_MASK 0x00000001   /* indicates if COMINIT or PHY ready detected */
#define PSSTS_SPD_MASK 0x000000F0
#define PSSTS_SPD_OFFSET 4
#define PSSTS_IPM_MASK 0x00000F00
#define PSSTS_IPM_OFFSET 8
#define PSSTS_IPM_NO_DEVICE 0x00000000
#define PSSTS_IPM_ACTIVE 0x00000001
#define PSSTS_IPM_PARTIAL 0x00000002
#define PSSTS_IPM_SLUMBER 0x00000006

#define PTFD_STS_MASK 0x000000FF
#define PTFD_STS_BSY_BIT 0x00000080
#define PTFD_STS_DRQ_BIT 0x00000008
#define PTFD_STS_ERR_BIT 0x00000001

#define PSERR_DIAG_X_BIT 0x04000000
#define PSERR_DIAG_F_BIT 0x02000000
#define PSERR_DIAG_W_BIT 0x00040000
#define PSERR_DIAG_N_BIT 0x00010000

#define PSCTL_DET_MASK 0x0000000F
#define PSCTL_DET_RESET 0x00000001
#define PSCTL_DET_OFFLINE 0x00000004
#define PSCTL_SPD_MASK 0x000000F0
#define PSCTL_IPM_MASK 0x00000F00
#define PSCTL_IPM_OFFSET 8

#define PSCTL_IPM_DISABLE_NONE 0x0
#define PSCTL_IPM_DISABLE_PARTIAL 0x1
#define PSCTL_IPM_DISABLE_SLUMBER 0x2
#define PSCTL_IPM_DISABLE_BOTH 0x3

#define PPHY_CTRL_CLR_RX_8B10B_ERR_CNT 0x00000100
#define PPHY_CTRL_TX_POLARITY 0x00000010
#define PPHY_CTRL_RX_POLARITY 0x00000020
#define PPHY_CTRL_TX_SWING 0x00000008
#define PPHY_TX_MARGIN_MASK 0x00000007

/* Custom PHY status bit fields */
#define PPHYSTAT_RX_8B10B_ERR_CNT_MASK 0x00000F00
#define PPHYSTAT_RX_8B10B_ERR_CNT_OFFSET 8
#define PPHYSTAT_SEND_D10_2_ST_BIT 0x00000002
#define PPHYSTAT_TRAIN_ENABLE_BIT 0x00000001

/* Port Interrupt Status Bits */
#define COLD_PORT_DETECT_STATUS ((UINT32_T)0x01 << 31)
#define TASK_FILE_ERROR_STATUS ((UINT32_T)0x01 << 30)
#define HOST_BUS_FATAL_ERROR_STATUS ((UINT32_T)0x01 << 29)
#define HOST_BUS_DATA_ERROR_STATUS ((UINT32_T)0x01 << 28)
#define INTERFACE_FATAL_ERROR_STATUS ((UINT32_T)0x01 << 27)
#define INTERFACE_NONFATAL_ERROR_STATUS ((UINT32_T)0x01 << 26)
#define OVERFLOW_STATUS ((UINT32_T)0x01 << 24)
#define WRONG_PORT_MULT_STATUS ((UINT32_T)0x01 << 23)
#define PHY_READY_CHANGE_STATUS                                                                                                            \
    ((UINT32_T)0x01 << 22) // This interrupt is not compatible with TUSB926x PHY.  Use AHCI interface errors to determine device disconnect.
#define DEVICE_MECH_PRESENCE_STATUS ((UINT32_T)0x01 << 7)
#define PORT_CONNECT_CHANGE_STATUS ((UINT32_T)0x01 << 6)
#define PRD_PROCESSED_STATUS ((UINT32_T)0x01 << 5)
#define UNKNOWN_FIS_INTR ((UINT32_T)0x01 << 4)
#define SET_DEVICE_BITS_FIS_INTR ((UINT32_T)0x01 << 3)
#define DMA_SETUP_FIS_INTR ((UINT32_T)0x01 << 2)
#define PIO_SETUP_FIS_INTR ((UINT32_T)0x01 << 1)
#define D2H_REGISTER_FIS_INTR ((UINT32_T)0x01)

enum {
    PORT_FATAL_ERROR_INTR = (TASK_FILE_ERROR_STATUS | HOST_BUS_FATAL_ERROR_STATUS | HOST_BUS_DATA_ERROR_STATUS |
                             INTERFACE_FATAL_ERROR_STATUS | UNKNOWN_FIS_INTR),

    PORT_DEFAULT_INTR_ENABLE = (PORT_FATAL_ERROR_INTR | OVERFLOW_STATUS |
                                /*PHY_READY_CHANGE_STATUS |*/ PORT_CONNECT_CHANGE_STATUS | SET_DEVICE_BITS_FIS_INTR | DMA_SETUP_FIS_INTR |
                                PIO_SETUP_FIS_INTR | D2H_REGISTER_FIS_INTR)
};

#define DEFAULT_ATA_SECTOR_SIZE 512    /* bytes */
#define DEFAULT_ATAPI_SECTOR_SIZE 2048 /* bytes */

#define H2D_REGISTER_FIS_TYPE 0x27

/* ATA device commands */
#define ATA_CMD_IDENTIFY_DEVICE 0xEC
#define ATA_CMD_IDENTIFY_PACKET_DEVICE 0xA1
#define ATA_CMD_SET_FEATURES 0xEF

#define ATA_CMD_PACKET 0xA0

#define ATA_CMD_READ_DMA 0xC8
#define ATA_CMD_READ_DMA_EXT 0x25
#define ATA_CMD_WRITE_DMA 0xCA
#define ATA_CMD_WRITE_DMA_EXT 0x35
#define ATA_CMD_WRITE_DMA_FUA_EXT 0x3D
#define ATA_CMD_READ_FPDMA_QUEUED 0x60
#define ATA_CMD_WRITE_FPDMA_QUEUED 0x61

#define ATA_CMD_CHECK_POWER_MODE 0xE5

#define ATA_CMD_FLUSH_CACHE 0xE7
#define ATA_CMD_FLUSH_CACHE_EXT 0xEA

#define ATA_CMD_READ_VERIFY_SECTORS 0x40
#define ATA_CMD_READ_VERIFY_SECTORS_EXT 0x42

#define ATA_CMD_READ_LOG_EXT 0x2F

#define ATA_CMD_STANDBY_IMMEDIATE 0xE0
#define ATA_CMD_IDLE_IMMEDIATE 0xE1
#define ATA_CMD_STANDBY 0xE2
#define ATA_CMD_IDLE 0xE3

#define ATA_CMD_DATA_SET_MGMT 0x06

#define ATA_CMD_TRUSTED_NON_DATA 0x5B
#define ATA_CMD_TRUSTED_RECEIVE_DMA 0x5D
#define ATA_CMD_TRUSTED_SEND_DMA 0x5F

#define ATA_CMD_SMART 0xB0

/* Misc defines */
#define SDB_FIS_CMD_SLOT_NUM 0xFD /* just a defined out-of-range slot number */

#define ATA_ERROR_STATUS_BIT 0x01

/*----------------------------------------------------------------------------+
| Structures                                                                  |
+----------------------------------------------------------------------------*/

/**
 * This structure stores ATA device information.
 */
typedef struct _ATA_DEVICE_INFO_T {
    UINT16_T wIdentifyDeviceInfo[256];
    UINT8_T bDeviceSignature[16];

    /* IDENTIFY DEVICE information */
    UINT16_T wSerialNum[10];  /* byte-swapped for valid ASCII string format */
    UINT16_T wFirmwareRev[4]; /* byte-swapped for valid ASCII string format */
    UINT16_T wModelNum[20];   /* byte-swapped for valid ASCII string format */
    BOOLEAN_T bPacketDevice;
    BOOLEAN_T bRemovableMediaDevice; /* Obsoleted in ATA8-ASC */
                                     //    UINT8_T      bMultiSectorNum;   /* number of sectors transferred per interrupt */

    UINT16_T wMDMA_ModesSupported;
    UINT16_T wUDMA_ModesSupported;
    UINT16_T wPIO_ModesSupported;

    UINT8_T bATA_MajorVersionNum; /* informative only - not req'd for code */
    UINT8_T bSATA_Gen;            /* Max signaling speed */

    BOOLEAN_T bLPM; /* indicates drive supports host initiated power management requests */

    BOOLEAN_T bDMA_SetupAutoActivateSupport;
    BOOLEAN_T bSoftwareSettingsPreservation;

    BOOLEAN_T bDMADIR; /* for ATAPI devices only */

    BOOLEAN_T bLBA48; /* indicates if we are dealing w/ 28-bit or 48-bit addressing - for ATA devices only */
    UINT64_T
    ddMaxLBA; /* derived from IDENTIFY DEVICE words 60-61 (28-bit) or 100-103 (48-bit) - for ATA devices only (equal to ddTrueMaxLBA unless we are emulating 4KB sectors)  */
    UINT64_T ddTrueMaxLBA; /* derived from IDENTIFY DEVICE words 60-61 (28-bit) or 100-103 (48-bit) - for ATA devices only */
    BOOLEAN_T bFUA;        /* Force Unit Available - for ATA devices and queued commands only */
    UINT16_T wWorldWideName[4];
    BOOLEAN_T bWorldWideNameValid;

    BOOLEAN_T bNCQ;
    UINT8_T bQueueDepth;       /* minimum queue depth supported by host and device (0 = depth of 1) */
    UINT8_T bActualQueueDepth; /* reported by ATA device */

    UINT32_T dSectorSize;       /* logical sector size in bytes (equal to dTrueSectorSize unless we are emulating 4KB sectors) */
    UINT32_T dTrueSectorSize;   /* logical sector size in bytes  */
    UINT8_T bPhysicalSectorExp; /* Word 106 bits 3:0 */
    UINT32_T dLogicalSectorsPerPhysicalSector;
    UINT32_T dPhysicalSectorSize; /* bytes */
    UINT16_T
    wLogicalSectorOffset; /* location of logical sector zero (equal to wTrueLogicalSectorOffset unless we are emulating 4KB sectors) */
    UINT16_T wTrueLogicalSectorOffset;

    UINT32_T
    wLowestAlignedLBA; /* equal to dLogicalSectorsPerPhysicalSector - wLogicalSectorOffset (only valid when emulating 4KB logical sectors) */
    BOOLEAN_T
    bLargeSectorEmulation; /* flag indicating emulation of 4KB logical sectors for supporting HDDs greater than 2.2 TB under WinXP */

    BOOLEAN_T bTRIMSupport; /* TRIM bit in the DATA SET MANAGEMENT command is supported */
    BOOLEAN_T bDRATSupport; /* Deterministic data in trimmed LBA range(s) is supported */
    BOOLEAN_T bRZATSupport; /* Read Zero After TRIM */
    UINT16_T wDataSetMgmtMaxBlocks;
    UINT16_T wMediaRotationRate;

    /* Other info */
    UINT8_T bCurrentCmdSlot; /* command slot of the last non-queued command issued */

    BOOLEAN_T bDeviceFault; /* Bit 5 of Status field */

    BOOLEAN_T bDeviceInitComplete; /* Indicates drive is ready to handle commands from SCSI layer */
    BOOLEAN_T bDeviceInitTimedOut;
    BOOLEAN_T bMediumChanged;

    BOOLEAN_T bPortReset; /* Indicates the port has been reset */

    BOOLEAN_T bCheckCondition[AHCI_NCQ_DEPTH]; /* for ATA PASS-THROUGH commands */

    BOOLEAN_T bWriteCmd[AHCI_NCQ_DEPTH];

    UINT32_T dSActive; /* Tracks bits set in the SACT register for queued R/W commands */

    /* Callbacks */
    void (*pAtaCmdCallback)(ATA_CMD_CALLBACK_T *pAtaCmdCbk);       /* for D2H register FIS or Set Device Bits FIS */
    void (*pAtaQueuedCmdCallback)(ATA_CMD_CALLBACK_T *pAtaCmdCbk); /* for DMA Setup FIS */
    void (*pAtaErrorCallback)(ATA_CMD_CALLBACK_T *pAtaCmdCbk);     /* for fatal SATA error interrupts */
    //void         *pData[AHCI_NCQ_DEPTH];
    void (*pSATAPortInitCallback)(UINT32_T port_num);

    /* Callback Queue */
    void (*pAtaCallbackQueue[ATA_CALLBACK_QUEUE_DEPTH])(ATA_CMD_CALLBACK_T *pAtaCmdCbk);
    ATA_CMD_CALLBACK_T ata_callback_data[ATA_CALLBACK_QUEUE_DEPTH];
    UINT32_T callback_index;
    UINT32_T callback_processing_index;
    BOOLEAN_T callback_pending[ATA_CALLBACK_QUEUE_DEPTH];

} ATA_DEVICE_INFO_T;

/**
 * This structure defines Host to Device Register FIS.
 */
typedef struct _REGISTER_FIS_H2D_T {
    UINT8_T FIS_type; /* 0x27 */
    UINT8_T CRRR_PMP;
    UINT8_T command;
    UINT8_T features;

    UINT8_T LBA_low;
    UINT8_T LBA_mid;
    UINT8_T LBA_high;
    UINT8_T device;

    UINT8_T LBA_low_exp;
    UINT8_T LBA_mid_exp;
    UINT8_T LBA_high_exp;
    UINT8_T features_exp;

    UINT8_T sector_cnt;
    UINT8_T sector_cnt_exp;
    UINT8_T rsvd0;
    UINT8_T control;

    UINT32_T rsvd1;
} REGISTER_FIS_H2D_T;

/**
 * This structure defines Device to Host Register FIS.
 */
typedef struct _REGISTER_FIS_D2H_T {
    UINT8_T FIS_type; /* 0x34 */
    UINT8_T CRRR_PMP;
    UINT8_T status;
    UINT8_T error;

    UINT8_T LBA_low;
    UINT8_T LBA_mid;
    UINT8_T LBA_high;
    UINT8_T device;

    UINT8_T LBA_low_exp;
    UINT8_T LBA_mid_exp;
    UINT8_T LBA_high_exp;
    UINT8_T rsvd0;

    UINT8_T sector_cnt;
    UINT8_T sector_cnt_exp;
    UINT8_T rsvd1;
    UINT8_T rsvd2;

    UINT32_T rsvd4;
} REGISTER_FIS_D2H_T;

/**
 * This structure used to pass ATA command information from the
 * SCSI block to the AHCI block.
 */
typedef struct _ATA_COMMAND_T {
    REGISTER_FIS_H2D_T fis;

    BOOLEAN_T bCheckCondition; /* for ATA PASS-THROUGH cmds */
    BOOLEAN_T bIsWriteCmd;
    BOOLEAN_T bUseMemoryWrapWindow; /* Should only be TRUE for R/W cmds */
    UINT32_T dDataByteCnt;
    /* ATAPI command block */
    UINT8_T atapi_cdb[16];

} ATA_COMMAND_T;

/*----------------------------------------------------------------------------+
| Externs                                                                     |
+----------------------------------------------------------------------------*/

extern UINT32_T gSATADeviceCount;
extern BOOLEAN_T gDiskActivity;
extern ATA_DEVICE_INFO_T ata_dev[NUM_AHCI_PORTS];

/*----------------------------------------------------------------------------+
| Function Prototypes                                                         |
+----------------------------------------------------------------------------*/

STATUS_T ahci_init(void);
void ahci_isr(void);
void ahci_rx_error_isr(void);

void ahci_register_ata_callbacks(void (*ata_cmd_callback)(ATA_CMD_CALLBACK_T *pAtaCmdCbk),
                                 void (*ata_queued_cmd_callback)(ATA_CMD_CALLBACK_T *pAtaCmdCbk),
                                 void (*ata_error_callback)(ATA_CMD_CALLBACK_T *pAtaCmdCbk));

void ahci_register_port_init_complete_callback(UINT32_T port_num, void (*medium_change_callback)(UINT32_T port_num));

STATUS_T ahci_build_cmd(UINT32_T port_num, ATA_COMMAND_T *ata_cmd, UINT32_T cmd_slot);
STATUS_T ahci_issue_cmd(UINT32_T port_num, UINT32_T cmd_slot, BOOLEAN_T is_queued_cmd);

void ahci_reset_lun(UINT32_T lun, BOOLEAN_T force_comreset);

//void ahci_power_down(UINT32_T port_num);

STATUS_T ahci_standby_immediate(UINT32_T port_num);

STATUS_T ahci_wait_complete(UINT32_T addr, UINT32_T cmplt_mask, UINT32_T cmplt_val, INT32_T timeout_ms);

STATUS_T ahci_identify_device(UINT32_T port_num);

#endif /*_AHCI_H_*/
