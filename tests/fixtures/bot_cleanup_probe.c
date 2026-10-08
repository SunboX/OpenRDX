/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 * SPDX-License-Identifier: AGPL-3.0-or-later
 * RAM-only regression fixture: no device or register access. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint32_t UINT32_T;
typedef int32_t INT32_T;
typedef unsigned char BOOLEAN_T;
typedef struct { UINT32_T dStatus; } TRB_T;
typedef struct {
    /* Match the signed remaining-byte counter in include/rdx_mount/usb_hal.h. */
    INT32_T dBytesRemaining;
    UINT32_T dXferLength;
    BOOLEAN_T bXferActive;
    TRB_T *pTRB;
} EP_INFO_T;
#define TRUE 1
#define FALSE 0
#define DEBUG(...) ((void)0)
#define INFO(...) ((void)0)
#define UMS_BOT_STATE_DATA_OUT 2
#define UMS_BOT_STATE_DATA_IN 1
#define BOT_BULK_OUT_ENDPT_NUM 1
#define BOT_BULK_IN_ENDPT_NUM 1
#define ENDPT_DIRECTION_OUT 0
#define ENDPT_DIRECTION_IN 0x80
#define TRB_STATUS_BUFFER_SIZE_MASK 0x00FFFFFF
static struct { EP_INFO_T ep_info_OUT[2]; } usb_dev;
static unsigned gBOT_state, out_cancels, in_cancels, out_stalls, in_stalls;
static TRB_T trb;

/** Records cancellation in RAM, including the endpoint's inactive transition. */
static void usb_hal_cancel_io_request(unsigned endpoint)
{
    if (endpoint & ENDPT_DIRECTION_IN) in_cancels++;
    else { out_cancels++; usb_dev.ep_info_OUT[1].bXferActive = FALSE; }
}
/** Records the requested stall direction without any USB call. */
static void ums_bot_stall(unsigned direction)
{ if (direction) in_stalls++; else out_stalls++; }

/* PRODUCTION_CLEANUP */

/** Initializes a single endpoint's local transfer-accounting fields. */
static void setup(unsigned state, BOOLEAN_T active, unsigned remaining,
                  unsigned residual)
{
    memset(&usb_dev, 0, sizeof(usb_dev));
    out_cancels = in_cancels = out_stalls = in_stalls = 0;
    gBOT_state = state; trb.dStatus = residual;
    usb_dev.ep_info_OUT[1].pTRB = &trb;
    usb_dev.ep_info_OUT[1].bXferActive = active;
    usb_dev.ep_info_OUT[1].dXferLength = 4096;
    usb_dev.ep_info_OUT[1].dBytesRemaining = remaining;
}

/** Exercises completed, partial, active, repeated and IN cleanup scenarios. */
int main(void)
{
    EP_INFO_T *ep = &usb_dev.ep_info_OUT[1];
    setup(2, FALSE, 0, 0); ums_bot_xfer_cleanup(TRUE);
    if (ep->dBytesRemaining || out_cancels || out_stalls || in_cancels != 1) return 1;
    setup(2, FALSE, 3584, 3584); ums_bot_xfer_cleanup(TRUE);
    if (ep->dBytesRemaining != 3584 || out_cancels || out_stalls) return 2;
    setup(2, TRUE, 4096, 1024); ums_bot_xfer_cleanup(TRUE);
    if (ep->dBytesRemaining != 1024 || out_cancels != 1 || out_stalls != 1 || ep->bXferActive) return 3;
    ums_bot_xfer_cleanup(TRUE);
    if (ep->dBytesRemaining != 1024 || out_cancels != 1 || out_stalls != 1) return 4;
    setup(2, TRUE, 4096, 0); ums_bot_xfer_cleanup(TRUE);
    if (ep->dBytesRemaining || ep->bXferActive || out_cancels || out_stalls) return 5;
    setup(2, TRUE, 4096, 1024); ums_bot_xfer_cleanup(FALSE);
    if (ep->dBytesRemaining != 1024 || out_cancels != 1 || out_stalls) return 6;
    setup(1, FALSE, 0, 0); ums_bot_xfer_cleanup(TRUE);
    if (out_cancels || out_stalls || in_cancels != 1 || in_stalls != 1) return 7;
    puts("PASS 7 BOT cleanup scenarios");
    return 0;
}
