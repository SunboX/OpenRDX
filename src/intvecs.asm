; SPDX-FileCopyrightText: 2019 Texas Instruments Incorporated
; All rights reserved.
; SPDX-FileCopyrightText: 2026 André Fiedler
;
; SPDX-License-Identifier: BSD-3-Clause AND AGPL-3.0-or-later

; Project integration: this low-level vector/exception layout
; follows the TUSB9261 vector-table contract required by OpenRDX.

;intvecs.asm
;
;

; Stack
         .global __stack
__stack: .usect  ".stack", 0, 4


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


;Interrupt Vectors

        .sect ".intvecs"

        .word _Stack_pv                 ;Initialize the Main Stack Pointer With This Value
        .word _c_int00                  ;Branch to c_int00 on Reset
        .word _NMI                      ;Non-Maskable Interrupt
        .word _Hard_fault         
        .word _MPU                      ;Memory Protection Unit
        .word _Bus_fault                ;Bus Fault a.k.a. Prefetch Abort or Data Abort
        .word _Usage_fault        
        .word _IO_PHANTOM_INT           ;
        .word _IO_PHANTOM_INT           ;Reserved Slots
        .word _IO_PHANTOM_INT           ;
        .word _IO_PHANTOM_INT           ;
        .word _SWI                      ;Software Interrupt
        .word _Debug_monitor            ;Debug Monitor Interrupt
        .word _IO_PHANTOM_INT           ;Reserved Slots
        .word _PSR                      ;Pendable Service Request
        .word _SYSTick                  ;System Tick Interrupt

        ;External Interrupts EXTINT[0] to EXTINT[239]

        .end



