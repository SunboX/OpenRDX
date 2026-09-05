; SPDX-FileCopyrightText: 2019 Texas Instruments Incorporated
; All rights reserved.
;
; SPDX-License-Identifier: BSD-3-Clause

;References for Interrupt Routines

        .global _c_int00
        .global _IO_PHANTOM_INT
        .global _NMI
        .global _Hard_fault
        .global _MPU
        .global _Bus_fault
        .global _Usage_fault
        .global _SWI
        .global _Debug_monitor
        .global _PSR
        .global _SYSTick
        .global _Stack_pv

;Exceptions ISRs

        .text

_Usage_fault:
     nop
     b    _Usage_fault

_IO_PHANTOM_INT:
     nop
     b    _IO_PHANTOM_INT

_NMI:
     nop
     b    _NMI

_SYSTick:
     nop
     b    _SYSTick

_MPU:
     nop
     b    _MPU

_PSR:
     nop
     b    _PSR

_Hard_fault:
     nop
     b    _Hard_fault

_Debug_monitor:
     nop
     b    _Debug_monitor

_SWI:
     nop
     b    _SWI

_Bus_fault:
     nop
     b    _Bus_fault
