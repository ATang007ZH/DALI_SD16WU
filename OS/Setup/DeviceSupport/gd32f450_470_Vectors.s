/*********************************************************************
*                     SEGGER Microcontroller GmbH                    *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 1995 - 2025 SEGGER Microcontroller GmbH                  *
*                                                                    *
*       Internet: segger.com  Support: support_embos@segger.com      *
*                                                                    *
**********************************************************************
*                                                                    *
*       embOS * Real time operating system                           *
*                                                                    *
*       Please note:                                                 *
*                                                                    *
*       Knowledge of this file may under no circumstances            *
*       be used to write a similar product or a real-time            *
*       operating system for in-house use.                           *
*                                                                    *
*       Thank you for your fairness !                                *
*                                                                    *
**********************************************************************
*                                                                    *
*       OS version: V5.20.0.0                                        *
*                                                                    *
**********************************************************************

-------------------------- END-OF-HEADER -----------------------------

File      : gd32f450_470_Vectors.s
Purpose   : Exception and interrupt vectors for gd32f450_470 devices.

Additional information:
  Preprocessor Definitions
    __NO_EXTERNAL_INTERRUPTS
      If defined,
        the vector table will contain only the internal exceptions
        and interrupts.
    __VECTORS_IN_RAM
      If defined,
        an area of RAM, large enough to store the vector table,
        will be reserved.

    __OPTIMIZATION_SMALL
      If defined,
        all weak definitions of interrupt handlers will share the
        same implementation.
      If not defined,
        all weak definitions of interrupt handlers will be defined
        with their own implementation.
*/
        .syntax unified

/*********************************************************************
*
*       Macros
*
**********************************************************************
*/

//
// Directly place a vector (word) in the vector table
//
.macro VECTOR Name=
        .section .vectors, "ax"
        .code 16
        .word \Name
.endm

//
// Declare an exception handler with a weak definition
//
.macro EXC_HANDLER Name=
        //
        // Insert vector in vector table
        //
        .section .vectors, "ax"
        .word \Name
        //
        // Insert dummy handler in init section
        //
        .section .init.\Name, "ax"
        .thumb_func
        .weak \Name
        .balign 2
\Name:
        1: b 1b   // Endless loop
.endm

//
// Declare an interrupt handler with a weak definition
//
.macro ISR_HANDLER Name=
        //
        // Insert vector in vector table
        //
        .section .vectors, "ax"
        .word \Name
        //
        // Insert dummy handler in init section
        //
#if defined(__OPTIMIZATION_SMALL)
        .section .init, "ax"
        .weak \Name
        .thumb_set \Name,Dummy_Handler
#else
        .section .init.\Name, "ax"
        .thumb_func
        .weak \Name
        .balign 2
\Name:
        1: b 1b   // Endless loop
#endif
.endm

//
// Place a reserved vector in vector table
//
.macro ISR_RESERVED
        .section .vectors, "ax"
        .word 0
.endm

//
// Place a reserved vector in vector table
//
.macro ISR_RESERVED_DUMMY
        .section .vectors, "ax"
        .word Dummy_Handler
.endm

/*********************************************************************
*
*       Externals
*
**********************************************************************
*/
        .extern __stack_end__
        .extern Reset_Handler
        .extern HardFault_Handler

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*  Setup of the vector table and weak definition of interrupt handlers
*
*/
        .section .vectors, "ax"
        .code 16
        .balign 512
        .global _vectors
_vectors:
        //
        // Internal exceptions and interrupts
        //
        VECTOR __stack_end__
        VECTOR Reset_Handler
        EXC_HANDLER NMI_Handler
        VECTOR HardFault_Handler
#ifdef __ARM_ARCH_6M__
        ISR_RESERVED
        ISR_RESERVED
        ISR_RESERVED
#else
        EXC_HANDLER MemManage_Handler
        EXC_HANDLER BusFault_Handler
        EXC_HANDLER UsageFault_Handler
#endif
        ISR_RESERVED
        ISR_RESERVED
        ISR_RESERVED
        ISR_RESERVED
        EXC_HANDLER SVC_Handler
#ifdef __ARM_ARCH_6M__
        ISR_RESERVED
#else
        EXC_HANDLER DebugMon_Handler
#endif
        ISR_RESERVED
        EXC_HANDLER PendSV_Handler
        EXC_HANDLER SysTick_Handler
        //
        // External interrupts
        //
#ifndef __NO_EXTERNAL_INTERRUPTS
        ISR_HANDLER WWDGT_IRQHandler
        ISR_HANDLER LVD_IRQHandler
        ISR_HANDLER TAMPER_STAMP_IRQHandler
        ISR_HANDLER RTC_WKUP_IRQHandler
        ISR_HANDLER FMC_IRQHandler
        ISR_HANDLER RCU_CTC_IRQHandler
        ISR_HANDLER EXTI0_IRQHandler
        ISR_HANDLER EXTI1_IRQHandler
        ISR_HANDLER EXTI2_IRQHandler
        ISR_HANDLER EXTI3_IRQHandler
        ISR_HANDLER EXTI4_IRQHandler
        ISR_HANDLER DMA0_Channel0_IRQHandler
        ISR_HANDLER DMA0_Channel1_IRQHandler
        ISR_HANDLER DMA0_Channel2_IRQHandler
        ISR_HANDLER DMA0_Channel3_IRQHandler
        ISR_HANDLER DMA0_Channel4_IRQHandler
        ISR_HANDLER DMA0_Channel5_IRQHandler
        ISR_HANDLER DMA0_Channel6_IRQHandler
        ISR_HANDLER ADC_IRQHandler
        ISR_HANDLER CAN0_TX_IRQHandler
        ISR_HANDLER CAN0_RX0_IRQHandler
        ISR_HANDLER CAN0_RX1_IRQHandler
        ISR_HANDLER CAN0_EWMC_IRQHandler
        ISR_HANDLER EXTI5_9_IRQHandler
        ISR_HANDLER TIMER0_BRK_TIMER8_IRQHandler
        ISR_HANDLER TIMER0_UP_TIMER9_IRQHandler
        ISR_HANDLER TIMER0_TRG_CMT_TIMER10_IRQHandler
        ISR_HANDLER TIMER0_Channel_IRQHandler
        ISR_HANDLER TIMER1_IRQHandler
        ISR_HANDLER TIMER2_IRQHandler
        ISR_HANDLER TIMER3_IRQHandler
        ISR_HANDLER I2C0_EV_IRQHandler
        ISR_HANDLER I2C0_ER_IRQHandler
        ISR_HANDLER I2C1_EV_IRQHandler
        ISR_HANDLER I2C1_ER_IRQHandler
        ISR_HANDLER SPI0_IRQHandler
        ISR_HANDLER SPI1_IRQHandler
        ISR_HANDLER USART0_IRQHandler
        ISR_HANDLER USART1_IRQHandler
        ISR_HANDLER USART2_IRQHandler
        ISR_HANDLER EXTI10_15_IRQHandler
        ISR_HANDLER RTC_Alarm_IRQHandler
        ISR_HANDLER USBFS_WKUP_IRQHandler
        ISR_HANDLER TIMER7_BRK_TIMER11_IRQHandler
        ISR_HANDLER TIMER7_UP_TIMER12_IRQHandler
        ISR_HANDLER TIMER7_TRG_CMT_TIMER13_IRQHandler
        ISR_HANDLER TIMER7_Channel_IRQHandler
        ISR_HANDLER DMA0_Channel7_IRQHandler
        ISR_HANDLER EXMC_IRQHandler
        ISR_HANDLER SDIO_IRQHandler
        ISR_HANDLER TIMER4_IRQHandler
        ISR_HANDLER SPI2_IRQHandler
        ISR_HANDLER UART3_IRQHandler
        ISR_HANDLER UART4_IRQHandler
        ISR_HANDLER TIMER5_DAC_IRQHandler
        ISR_HANDLER TIMER6_IRQHandler
        ISR_HANDLER DMA1_Channel0_IRQHandler
        ISR_HANDLER DMA1_Channel1_IRQHandler
        ISR_HANDLER DMA1_Channel2_IRQHandler
        ISR_HANDLER DMA1_Channel3_IRQHandler
        ISR_HANDLER DMA1_Channel4_IRQHandler
        ISR_HANDLER ENET_IRQHandler
        ISR_HANDLER ENET_WKUP_IRQHandler
        ISR_HANDLER CAN1_TX_IRQHandler
        ISR_HANDLER CAN1_RX0_IRQHandler
        ISR_HANDLER CAN1_RX1_IRQHandler
        ISR_HANDLER CAN1_EWMC_IRQHandler
        ISR_HANDLER USBFS_IRQHandler
        ISR_HANDLER DMA1_Channel5_IRQHandler
        ISR_HANDLER DMA1_Channel6_IRQHandler
        ISR_HANDLER DMA1_Channel7_IRQHandler
        ISR_HANDLER USART5_IRQHandler
        ISR_HANDLER I2C2_EV_IRQHandler
        ISR_HANDLER I2C2_ER_IRQHandler
        ISR_HANDLER USBHS_EP1_Out_IRQHandler
        ISR_HANDLER USBHS_EP1_In_IRQHandler
        ISR_HANDLER USBHS_WKUP_IRQHandler
        ISR_HANDLER USBHS_IRQHandler
        ISR_HANDLER DCI_IRQHandler
        ISR_RESERVED
        ISR_HANDLER TRNG_IRQHandler
        ISR_HANDLER FPU_IRQHandler
        ISR_HANDLER UART6_IRQHandler
        ISR_HANDLER UART7_IRQHandler
        ISR_HANDLER SPI3_IRQHandler
        ISR_HANDLER SPI4_IRQHandler
        ISR_HANDLER SPI5_IRQHandler
        ISR_RESERVED
        ISR_HANDLER TLI_IRQHandler
        ISR_HANDLER TLI_ER_IRQHandler
        ISR_HANDLER IPA_IRQHandler
#endif
        //
        .section .vectors, "ax"
_vectors_end:

#ifdef __VECTORS_IN_RAM
        //
        // Reserve space with the size of the vector table
        // in the designated RAM section.
        //
        .section .vectors_ram, "ax"
        .balign 512
        .global _vectors_ram

_vectors_ram:
        .space _vectors_end - _vectors, 0
#endif

/*********************************************************************
*
*  Dummy handler to be used for reserved interrupt vectors
*  and weak implementation of interrupts.
*
*/
        .section .init.Dummy_Handler, "ax"
        .thumb_func
        .weak Dummy_Handler
        .balign 2
Dummy_Handler:
        1: b 1b   // Endless loop


/*************************** End of file ****************************/
