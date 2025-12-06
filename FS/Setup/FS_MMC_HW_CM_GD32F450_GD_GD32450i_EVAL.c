/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
*                           www.segger.com                           *
**********************************************************************

-------------------------- END-OF-HEADER -----------------------------

File    : FS_MMC_HW_CM_GD32F450_GD_GD32450i_EVAL.c
Purpose : Hardware layer for MMC/SD driver in card mode
          for the GigaDevice GD32450i-EVAL evaluation board.
Literature:
  [1] Datasheet GD32F450xx ARM Cortex-M4 32-bit MCU
    (\\FILESERVER\Techinfo\Company\GigaDevice\MCUs\GD32F4\GD32F450xx_Datasheet_Rev1.1.pdf)
  [2] User manual GD32F4xx ARM Cortex-M4 32-bit MCU for GD32F405xx, GD32F407xx and GD32F450xx
    (\\FILESERVER\Techinfo\Company\GigaDevice\MCUs\GD32F4\GD32F4xx_User_Manual_Rev2.6.pdf)
  [3] User Manual GD32450I-EVAL
    (\\FILESERVER\Techinfo\Company\GigaDevice\MCUs\GD32F4\EvalBoard\GD32450I-EVAL\GD32450I-EVAL_User_Manual-V1.0.pdf)
*/

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/
#include "FS.h"

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/
#ifndef   FS_MMC_HW_CM_SDIO_CLK
  #define FS_MMC_HW_CM_SDIO_CLK             48000     // Clock of MMC module in kHz
#endif

#ifndef   FS_MMC_HW_CM_MAX_SD_CLK
  #define FS_MMC_HW_CM_MAX_SD_CLK           48000     // Maximum transfer speed
#endif

#ifndef   FS_MMC_HW_CM_USE_OS
  #define FS_MMC_HW_CM_USE_OS               0         // Selects the operating mode: 1 - event-driven, 0 - polling
#endif

#ifndef   FS_MMC_HW_CM_POWER_GOOD_DELAY
  #define FS_MMC_HW_CM_POWER_GOOD_DELAY     50        // Number of  milliseconds to wait for the power supply of SD card to become ready
#endif

#ifndef   FS_MMC_HW_CM_ALIGNED_DMA_BURSTS
  #define FS_MMC_HW_CM_ALIGNED_DMA_BURSTS   0         // When set to 1 the DMA burst is aligned to the address and the number of bytes to be transferred.
#endif

#ifndef   FS_MMC_HW_CM_DIV_SD_CLK
  #define FS_MMC_HW_CM_DIV_SD_CLK           0         // Can be used to avoid underrun/overrun errors by slowing down the SD clock.
                                                      // This value is added to the current SD clock divider (SDIO_CLKCR.CLKDIV)
#endif

#ifndef   FS_MMC_HW_CM_CYCLES_PER_1MS
  #define FS_MMC_HW_CM_CYCLES_PER_1MS       30000     // Number of software cycles required to generate a 1ms delay
#endif

#ifndef   FS_MMC_HW_CM_HCLK
  #define FS_MMC_HW_CM_HCLK                 200000000
#endif

#ifndef   FS_MMC_HW_CM_PCLK2
  #define FS_MMC_HW_CM_PCLK2                (FS_MMC_HW_CM_HCLK / 2)
#endif

/*********************************************************************
*
*       #include section, conditional
*
**********************************************************************
*/
#if FS_MMC_HW_CM_USE_OS
  #include "RTOS.h"
  #include "FS_OS.h"
  #include "gd32f4xx.h"
#endif

/*********************************************************************
*
*       Defines, non-configurable
*
**********************************************************************
*/

/*********************************************************************
*
*       SDIO interface registers
*/
#define SDIO_BASE_ADDR          0x40012C00uL
#define SDIO_POWER              (*(volatile U32 *)(SDIO_BASE_ADDR + 0x00))
#define SDIO_CLKCR              (*(volatile U32 *)(SDIO_BASE_ADDR + 0x04))
#define SDIO_ARG                (*(volatile U32 *)(SDIO_BASE_ADDR + 0x08))
#define SDIO_CMD                (*(volatile U32 *)(SDIO_BASE_ADDR + 0x0C))
#define SDIO_RESPCMD            (*(volatile U32 *)(SDIO_BASE_ADDR + 0x10))
#define SDIO_RESP1              (*(volatile U32 *)(SDIO_BASE_ADDR + 0x14))
#define SDIO_RESP2              (*(volatile U32 *)(SDIO_BASE_ADDR + 0x18))
#define SDIO_RESP3              (*(volatile U32 *)(SDIO_BASE_ADDR + 0x1C))
#define SDIO_RESP4              (*(volatile U32 *)(SDIO_BASE_ADDR + 0x20))
#define SDIO_DTIMER             (*(volatile U32 *)(SDIO_BASE_ADDR + 0x24))
#define SDIO_DLEN               (*(volatile U32 *)(SDIO_BASE_ADDR + 0x28))
#define SDIO_DCTRL              (*(volatile U32 *)(SDIO_BASE_ADDR + 0x2C))
#define SDIO_DCOUNT             (*(volatile U32 *)(SDIO_BASE_ADDR + 0x30))
#define SDIO_STA                (*(volatile U32 *)(SDIO_BASE_ADDR + 0x34))
#define SDIO_ICR                (*(volatile U32 *)(SDIO_BASE_ADDR + 0x38))
#define SDIO_MASK               (*(volatile U32 *)(SDIO_BASE_ADDR + 0x3C))
#define SDIO_FIFOCNT            (*(volatile U32 *)(SDIO_BASE_ADDR + 0x48))
#define SDIO_FIFO               (*(volatile U32 *)(SDIO_BASE_ADDR + 0x80))

/*********************************************************************
*
*       Reset and clock unit registers
*/
#define RCU_BASE_ADDR           0x40023800uL
#define RCU_AHB1RSTR            (*(volatile U32 *)(RCU_BASE_ADDR + 0x10))
#define RCU_APB2RSTR            (*(volatile U32 *)(RCU_BASE_ADDR + 0x24))
#define RCU_AHB1ENR             (*(volatile U32 *)(RCU_BASE_ADDR + 0x30))
#define RCU_APB2ENR             (*(volatile U32 *)(RCU_BASE_ADDR + 0x44))

/*********************************************************************
*
*       Port C registers
*/
#define GPIOC_BASE_ADDR         0x40020800uL
#define GPIOC_MODER             (*(volatile U32 *)(GPIOC_BASE_ADDR + 0x00))
#define GPIOC_OTYPER            (*(volatile U32 *)(GPIOC_BASE_ADDR + 0x04))
#define GPIOC_OSPEEDR           (*(volatile U32 *)(GPIOC_BASE_ADDR + 0x08))
#define GPIOC_PUPDR             (*(volatile U32 *)(GPIOC_BASE_ADDR + 0x0C))
#define GPIOC_IDR               (*(volatile U32 *)(GPIOC_BASE_ADDR + 0x10))
#define GPIOC_AFRL              (*(volatile U32 *)(GPIOC_BASE_ADDR + 0x20))
#define GPIOC_AFRH              (*(volatile U32 *)(GPIOC_BASE_ADDR + 0x24))

/*********************************************************************
*
*       Port D registers
*/
#define GPIOD_BASE_ADDR         0x40020C00uL
#define GPIOD_MODER             (*(volatile U32 *)(GPIOD_BASE_ADDR + 0x00))
#define GPIOD_OTYPER            (*(volatile U32 *)(GPIOD_BASE_ADDR + 0x04))
#define GPIOD_OSPEEDR           (*(volatile U32 *)(GPIOD_BASE_ADDR + 0x08))
#define GPIOD_PUPDR             (*(volatile U32 *)(GPIOD_BASE_ADDR + 0x0C))
#define GPIOD_AFRL              (*(volatile U32 *)(GPIOD_BASE_ADDR + 0x20))

/*********************************************************************
*
*       DMA 2 registers
*/
#define DMA2_BASE_ADDR          0x40026400uL
#define DMA2_LISR               (*(volatile U32 *)(DMA2_BASE_ADDR + 0))
#define DMA2_HISR               (*(volatile U32 *)(DMA2_BASE_ADDR + 4))
#define DMA2_LIFCR              (*(volatile U32 *)(DMA2_BASE_ADDR + 8))
#define DMA2_HIFCR              (*(volatile U32 *)(DMA2_BASE_ADDR + 12))
#define DMA2_S3CR               (*(volatile U32 *)(DMA2_BASE_ADDR + 24 * 3 + 16))
#define DMA2_S3NDTR             (*(volatile U32 *)(DMA2_BASE_ADDR + 24 * 3 + 20))
#define DMA2_S3PAR              (*(volatile U32 *)(DMA2_BASE_ADDR + 24 * 3 + 24))
#define DMA2_S3M0AR             (*(volatile U32 *)(DMA2_BASE_ADDR + 24 * 3 + 28))
#define DMA2_S3FCR              (*(volatile U32 *)(DMA2_BASE_ADDR + 24 * 3 + 36))

/*********************************************************************
*
*       Reset and clock bits for the peripherals used by the driver
*/
#define AHB1ENR_DMA2EN          (1uL << 22)
#define AHB1ENR_PORTCEN         (1uL << 2)
#define AHB1ENR_PORTDEN         (1uL << 3)
#define APB2ENR_SDIOEN          (1uL << 11)
#define AHB1RSTR_DMA2RST        (1uL << 22)
#define APB2RSTR_SDIORST        (1uL << 11)

/*********************************************************************
*
*       SDIO status register
*/
#define STA_CCRCFAIL            (1uL << 0)
#define STA_DCRCFAIL            (1uL << 1)
#define STA_CTIMEOUT            (1uL << 2)
#define STA_DTIMEOUT            (1uL << 3)
#define STA_TXUNDERR            (1uL << 4)
#define STA_RXOVERR             (1uL << 5)
#define STA_CMDREND             (1uL << 6)
#define STA_CMDSENT             (1uL << 7)
#define STA_DATAEND             (1uL << 8)
#define STA_STBITERR            (1uL << 9)
#define STA_DBCKEND             (1uL << 10)
#define STA_CMDACT              (1uL << 11)
#define STA_TXACT               (1uL << 12)
#define STA_RXACT               (1uL << 13)

/*********************************************************************
*
*       SDIO data control register
*/
#define DCTRL_DTEN              (1uL << 0)
#define DCTRL_DTDIR             (1uL << 1)
#define DCTRL_DTMODE            (1uL << 2)
#define DCTRL_DMAEN             (1uL << 3)
#define DCTRL_DBLOCKSIZE_SHIFT  4uL

/*********************************************************************
*
*       SDIO clock control register
*/
#define CLKCR_CLKEN             (1uL   << 8)
#define CLKCR_CLK_PWRSAV        (0x1uL <<  9)
#define CLKCR_CLK_BYPASS        (0x1uL << 10)
#define CLKCR_WIDBUS_MASK       (0x3uL << 11)
#define CLKCR_WIDBUS_4BIT       (0x1uL << 11)
#define CLKCR_WIDBUS_8BIT       (0x2uL << 11)
#define CLKCR_HWFC_EN           (1UL   << 14)

/*********************************************************************
*
*       SDIO command register
*/
#define CMD_CMD_MASK            (0x3FuL)
#define CMD_WAITRESP_SHORT      (1uL << 6)
#define CMD_WAITRESP_LONG       (3uL << 6)
#define CMD_CPSMEN              (1uL << 10)
#define CMD_ENDCMDCMPLT         (1uL << 12)
#define CMD_NIEN                (1uL << 13)
#define CMD_WAITPEND            (1uL <<  9)

/*********************************************************************
*
*       SDIO interrupt control register
*/
#define ICR_CCRCFAIL            (1uL << 0)
#define ICR_DCRCFAIL            (1uL << 1)
#define ICR_CTIMEOUT            (1uL << 2)
#define ICR_DTIMEOUT            (1uL << 3)
#define ICR_TXUNDERR            (1uL << 4)
#define ICR_RXOVERR             (1uL << 5)
#define ICR_CMDREND             (1uL << 6)
#define ICR_CMDSENT             (1uL << 7)
#define ICR_DATAEND             (1uL << 8)
#define ICR_STBITERR            (1uL << 9)
#define ICR_DBCKEND             (1uL << 10)
#define ICR_SDIOIT              (1uL << 22)
#define ICR_CEATAEND            (1uL << 23)
#define ICR_MASK_STATIC         (ICR_CCRCFAIL | \
                                 ICR_DCRCFAIL | \
                                 ICR_CTIMEOUT | \
                                 ICR_DTIMEOUT | \
                                 ICR_TXUNDERR | \
                                 ICR_RXOVERR  | \
                                 ICR_CMDREND  | \
                                 ICR_CMDSENT  | \
                                 ICR_DATAEND  | \
                                 ICR_STBITERR | \
                                 ICR_DBCKEND  | \
                                 ICR_SDIOIT   | \
                                 ICR_CEATAEND)

/*********************************************************************
*
*       SDIO interrupt mask register
*/
#define MASK_CCRCFAILIE         (1uL << 0)
#define MASK_DCRCFAILIE         (1uL << 1)
#define MASK_CTIMEOUTIE         (1uL << 2)
#define MASK_DTIMEOUTIE         (1uL << 3)
#define MASK_TXUNDERRIE         (1uL << 4)
#define MASK_RXOVERRIE          (1uL << 5)
#define MASK_CMDRENDIE          (1uL << 6)
#define MASK_CMDSENTIE          (1uL << 7)
#define MASK_DATAENDIE          (1uL << 8)
#define MASK_STBITERRIE         (1uL << 9)
#define MASK_DBCKENDIE          (1uL <<10)
#define MASK_ALL                (MASK_CCRCFAILIE | \
                                 MASK_DCRCFAILIE | \
                                 MASK_CTIMEOUTIE | \
                                 MASK_DTIMEOUTIE | \
                                 MASK_TXUNDERRIE | \
                                 MASK_RXOVERRIE  | \
                                 MASK_CMDRENDIE  | \
                                 MASK_CMDSENTIE  | \
                                 MASK_DATAENDIE  | \
                                 MASK_STBITERRIE | \
                                 MASK_DBCKENDIE)

/*********************************************************************
*
*       SDIO data length register
*/
#define DLEN_DATALENGTH_MASK    (0x03FFFFFFuL)

/*********************************************************************
*
*       GPIO bit positions of SD card lines
*
*/
#define SD_D0_BIT               8     // Port C
#define SD_D1_BIT               9     // Port C
#define SD_D2_BIT               10    // Port C
#define SD_D3_BIT               11    // Port C
#define SD_CLK_BIT              12    // Port C
#define SD_CMD_BIT              2     // Port D

/*********************************************************************
*
*       DMA2 related defines
*/
#define LIFCR_CFEIF3            (1uL << 22)
#define LIFCR_CDMEIF3           (1uL << 24)
#define LIFCR_CTCIF3            (1uL << 25)
#define LIFCR_CHTIF3            (1uL << 26)
#define LIFCR_CTEIF3            (1uL << 27)
#define LISR_FEIF3              (1uL << 22)
#define LISR_TEIF3              (1uL << 25)
#define LISR_TCIF3              (1uL << 27)
#define S3CR_EN                 (1uL << 0)
#define S3CR_TEIE               (1uL << 2)
#define S3CR_TCIE               (1uL << 4)
#define S3CR_DIR_MASK           (3uL << 6)
#define S3CR_DIR_M2P            (1uL << 6)
#define S3CR_MINC               (1uL << 10)
#define S3CR_PSIZE_32BIT        (2uL << 11)
#define S3CR_MSIZE_32BIT        (2uL << 13)
#define S3CR_PRIO_HIGH          (3uL << 16)
#define S3CR_CHSEL_CH4          (4uL << 25)
#define S3CR_PFCTRL             (1uL << 5)
#define S3CR_PBURST_INCR4       (1uL << 21)
#define S3CR_MBURST_INCR4       (1uL << 23)
#define S3CR_MBURST_NONE        (0uL << 23)
#define S3CR_MBURST_MASK        (3uL << 23)
#define S3FCR_DMDIS             (1uL << 2)
#define S3FCR_FTH_FULL          (3uL << 0)

/*********************************************************************
*
*       Misc. defines
*/
#define PERIPHERAL_TO_MEMORY    0
#define MEMORY_TO_PERIPHERAL    1
#define DMA_MAX_NUM_TRANSFERS   65535
#define MAX_BLOCK_SIZE          512     // Maximum number of bytes in a transferred data block
#define MAX_NUM_BLOCKS          ((DMA_MAX_NUM_TRANSFERS * 4) / MAX_BLOCK_SIZE)  // DMA transfers 4 bytes at once
#define WAIT_TIMEOUT_MS         1000
#define WAIT_TIMEOUT_CYCLES     (WAIT_TIMEOUT_MS * FS_MMC_HW_CM_CYCLES_PER_1MS)
#define SDIO_PRIO               15
#define DMA_PRIO                15

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static U8               _IgnoreCRC;
static U16              _BlockSize;
static U16              _NumBlocks;
static U32              _DataBlockSize;
static void           * _pBuffer;
static U32              _NumLoopsWriteRegDelay;
static U8               _IsR1Busy;
#if FS_MMC_HW_CM_USE_OS
  static volatile U32   _StatusSDIO;
  static volatile U32   _StatusDMA;
#endif
#if FS_MMC_HW_CM_DIV_SD_CLK
  static U32            _RegValueCLKCR;
#endif
#if (FS_VERSION > 40405)
  static U8             _RepeatSame;
#endif

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _ld
*/
static unsigned _ld(U32 Value) {
  unsigned i;

  for (i = 0; i < 16u; i++) {
    if ((1uL << i) == Value) {
      break;
    }
  }
  return i;
}

/*********************************************************************
*
*       _Delay1ms
*
*  Function description
*    Blocks the program execution for about 1ms.
*    The number of loops must be adjusted according to the CPU speed.
*/
static void _Delay1ms(void) {
#if FS_MMC_HW_CM_USE_OS
  OS_Delay(1);
#else
  volatile unsigned NumLoops;

  NumLoops = FS_MMC_HW_CM_CYCLES_PER_1MS;
  do {
    ;
  } while (--NumLoops != 0u);
#endif
}

#if FS_MMC_HW_CM_DIV_SD_CLK

/*********************************************************************
*
*       _IsIntRAM
*
*  Function description
*    Checks if the address is located into internal RAM.
*/
static int _IsIntRAM(U32 Addr) {
  if ((Addr >= 0x20000000uL) && (Addr < 0x40000000uL)) {
    return 1;
  }
  return 0;
}

#endif  // FS_MMC_HW_CM_DIV_SD_CLK

/*********************************************************************
*
*       _StartDMATransfer
*/
static void _StartDMATransfer(U32 * pMemory, U32 * pPripheral, U8 Direction, U32 NumBytes) {
  FS_USE_PARA(NumBytes);
  DMA2_S3CR   &= ~S3CR_EN;          // Stop the data transfer
  while (DMA2_S3CR & S3CR_EN) {     // Wait for the stream to switch off
    ;
  }
  DMA2_S3PAR  = (U32)pPripheral;    // Periphery data register address
  DMA2_S3M0AR = (U32)pMemory;       // Memory buffer address
  DMA2_S3NDTR = 0;
  DMA2_LIFCR  = LIFCR_CDMEIF3       // Clear any pending interrupts.
              | LIFCR_CTEIF3
              | LIFCR_CHTIF3
              | LIFCR_CTCIF3
              | LIFCR_CFEIF3
              ;
  DMA2_S3CR   &= ~S3CR_DIR_MASK;
#if FS_MMC_HW_CM_USE_OS
  DMA2_S3CR   |= S3CR_TEIE
              |  S3CR_TCIE
              ;
  _StatusDMA  = 0;
#endif
  if (Direction == MEMORY_TO_PERIPHERAL) {
    DMA2_S3CR |= S3CR_DIR_M2P;
  }
  //
  // Set the correct burst size based on the number of bytes transferred.
  //
  DMA2_S3CR   &= ~S3CR_MBURST_MASK;
  if ((NumBytes & 0xFFuL) == 0) {
    DMA2_S3CR |= 0
              | S3CR_PBURST_INCR4   // Burst transfer on peripheral side
              | S3CR_MBURST_INCR4   // Burst transfer on memory side
              ;
  }
#if FS_MMC_HW_CM_ALIGNED_DMA_BURSTS
  //
  // The memory address must be aligned to burst size.
  // If this is not the case use single transfers.
  //
  DMA2_S3CR   &= ~S3CR_MBURST_MASK;
  if (((U32)pMemory & 0xFuL)  == 0 &&   // Can DMA perform bursts of 16 bytes?
      ((NumBytes    & 0xFFuL) == 0)) {
    DMA2_S3CR |= S3CR_MBURST_INCR4;
  } else {
    DMA2_S3CR |= S3CR_MBURST_NONE;
  }
#endif // FS_MMC_HW_CM_ALIGNED_DMA_BURSTS
#if FS_MMC_HW_CM_DIV_SD_CLK
  _RegValueCLKCR = 0;
  if (_IsIntRAM((U32)pMemory) == 0) {
    U32 v;

    //
    // Slow down the SD clock if external RAM or internal flash is accessed
    // in order to prevent DMA FIFO overrun/underrun errors.
    //
    v = SDIO_CLKCR;
    _RegValueCLKCR = v;
    v &= ~CLKCR_CLK_BYPASS;
    v += FS_MMC_HW_CM_DIV_SD_CLK;
    SDIO_CLKCR = v;
  }
#endif  // FS_MMC_HW_CM_DIV_SD_CLK
#if (FS_VERSION > 40405)
  if (_RepeatSame) {
    DMA2_S3CR &= ~S3CR_MINC;      // Do not increment memory pointer when filling with the same 32-bit value.
  } else {
    DMA2_S3CR |=  S3CR_MINC;      // Increment the memory pointer when writing entire blocks.
  }
#endif // FS_VERSION > 40405
  DMA2_S3CR   |= S3CR_EN;               // Start the data transfer
}

#if FS_MMC_HW_CM_USE_OS

/**********************************************************
*
*       SDIO_IRQHandler
*
*   Function description
*     Handles the SDIO interrupt.
*/
void SDIO_IRQHandler(void);
void SDIO_IRQHandler(void) {
  U32 Status;

  OS_EnterInterrupt();                        // Inform embOS that interrupt code is running.
  Status       = SDIO_STA;
  //
  // Save the status to a static variable and check it in the task.
  //
  SDIO_ICR     = Status & ICR_MASK_STATIC;    // Clear the static flags to prevent further interrupts.
  _StatusSDIO &= ICR_MASK_STATIC;             // Clear the dynamic flags
  _StatusSDIO |= Status;
  FS_X_OS_Signal();                           // Wake up the task.
  OS_LeaveInterrupt();                        // Inform embOS that interrupt code is left.
}

/**********************************************************
*
*       DMA1_Channel3_IRQHandler
*
*   Function description
*     Handles the DMA interrupt.
*/
void DMA1_Channel3_IRQHandler(void);
void DMA1_Channel3_IRQHandler(void) {
  U32 Status;

  OS_EnterInterrupt();          // Inform embOS that interrupt code is running.
  Status      = DMA2_LISR;
  Status     &= 0               // Make sure that we clear only the flags assigned to the DMA stream we use.
             | LIFCR_CFEIF3
             | LIFCR_CDMEIF3
             | LIFCR_CTCIF3
             | LIFCR_CHTIF3
             | LIFCR_CTEIF3
             ;
  DMA2_LIFCR  = Status;         // Clear pending interrupt flags.
  _StatusDMA |= Status;         // Save the status to a static variable and check it in the task.
  FS_X_OS_Signal();             // Wake up the task.
  OS_LeaveInterrupt();          // Inform embOS that interrupt code is left.
}

#endif

/*********************************************************************
*
*       _WriteRegDelayed
*
*  Function description
*    Waits before writing to a register as specified in the datasheet:
*      "After a data write, data cannot be written to this register for three SDIOCLK
*      (48 MHz) clock periods plus two FS_MMC_HW_CM_PCLK2 clock periods."
*/
static void _WriteRegDelayed(volatile U32 * pAddr, U32 Value) {
  volatile U32 NumLoops;

  NumLoops  = _NumLoopsWriteRegDelay;
  do {
    ;
  } while (--NumLoops);
  *pAddr = Value;
}

/*********************************************************************
*
*       _WaitForInactive
*/
static void _WaitForInactive(void) {
  U32 Status;
  U32 v;
  U32 TimeOut;

  TimeOut = WAIT_TIMEOUT_CYCLES;
  for (;;) {
    Status = SDIO_STA;
    if (((Status & STA_CMDACT) == 0) &&
        ((Status & STA_TXACT)  == 0) &&
        ((Status & STA_RXACT)  == 0)) {
      if (_IsR1Busy == 0) {
        break;
      }
      //
      // The SDIO host controller is not able to handle R1 responses
      // with busy signaling therefore we have to check here the
      // level of the DAT0 line. The SD / MMC device drives the DAT0
      // line to LOW if busy.
      // The SD / MMC devices require to be clocked in order to release
      // the DAT0 line but the SDIO host controller is configured clock
      // the SD/ MMC device only when a command or data is transferred
      // (to prevent DMA issues). For this reason we have to temporarily
      // disable this mode so that the SDIO host controller
      // clocks the SD / MMC device while we are checking the status
      // of the DAT0 line.
      //
      if (GPIOC_IDR & (1uL << SD_D0_BIT)) {
        _IsR1Busy = 0;
        if ((SDIO_CLKCR & CLKCR_CLK_PWRSAV) == 0) {
          v = SDIO_CLKCR | CLKCR_CLK_PWRSAV;
          _WriteRegDelayed(&SDIO_CLKCR, v);
        }
        break;
      }
      if (SDIO_CLKCR & CLKCR_CLK_PWRSAV) {
        v = SDIO_CLKCR & ~CLKCR_CLK_PWRSAV;
        _WriteRegDelayed(&SDIO_CLKCR, v);
      }
    }
    if (--TimeOut == 0u) {
      break;
    }
  }
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       _HW_Init
*
*  Function description
*    Initialize the SD / MMC host controller.
*
*  Parameters
*    Unit     Index of the SD / MMC host controller (0-based).
*
*  Notes
*     (1) Flow control should not be enabled as it causes glitches on the clock line (see [3])
*/
static void _HW_Init(U8 Unit) {
  unsigned ms;
  U32      Delay_ns;
  U32      Inst_ns;

  FS_USE_PARA(Unit);
  //
  // Reset SDIO
  //
  RCU_APB2RSTR |= APB2RSTR_SDIORST;
  RCU_APB2RSTR &= ~APB2RSTR_SDIORST;

  //
  // Enable GPIOs, DMA and SDIO
  //
  RCU_AHB1ENR |= AHB1ENR_DMA2EN
              |  AHB1ENR_PORTCEN
              |  AHB1ENR_PORTDEN
              ;
  RCU_APB2ENR |= APB2ENR_SDIOEN
              ;
  //
  // D0, D1, D2, D3, CLK lines are controlled by SDIO
  //
  GPIOC_MODER   &= ~((3uL << (SD_D0_BIT  << 1)) |
                     (3uL << (SD_D1_BIT  << 1)) |
                     (3uL << (SD_D2_BIT  << 1)) |
                     (3uL << (SD_D3_BIT  << 1)) |
                     (3uL << (SD_CLK_BIT << 1)));
  GPIOC_MODER   |=   (2uL << (SD_D0_BIT  << 1))
                |    (2uL << (SD_D1_BIT  << 1))
                |    (2uL << (SD_D2_BIT  << 1))
                |    (2uL << (SD_D3_BIT  << 1))
                |    (2uL << (SD_CLK_BIT << 1))
                ;
  GPIOC_OTYPER  &= ~((1uL <<  SD_D0_BIT) |
                     (1uL <<  SD_D1_BIT) |
                     (1uL <<  SD_D2_BIT) |
                     (1uL <<  SD_D3_BIT) |
                     (1uL <<  SD_CLK_BIT));
  GPIOC_PUPDR   &= ~((3uL << (SD_D0_BIT  << 1)) |
                     (3uL << (SD_D1_BIT  << 1)) |
                     (3uL << (SD_D2_BIT  << 1)) |
                     (3uL << (SD_D3_BIT  << 1)) |
                     (3uL << (SD_CLK_BIT << 1)));
  GPIOC_PUPDR   |=   (1uL << (SD_D0_BIT  << 1))   // The data transfer works reliably only if the pull-ups are enabled.
                |    (1uL << (SD_D1_BIT  << 1))
                |    (1uL << (SD_D2_BIT  << 1))
                |    (1uL << (SD_D3_BIT  << 1))
                |    (1uL << (SD_CLK_BIT << 1))
                ;
  GPIOC_AFRH    &= ~((0xFuL << ((SD_D0_BIT - 8)  << 2)) |
                     (0xFuL << ((SD_D1_BIT - 8) << 2))  |
                     (0xFuL << ((SD_D2_BIT - 8)  << 2)) |
                     (0xFuL << ((SD_D3_BIT - 8)  << 2)) |
                     (0xFuL << ((SD_CLK_BIT - 8) << 2)));
  GPIOC_AFRH    |=   (12uL  << ((SD_D0_BIT - 8)  << 2))
                |    (12uL  << ((SD_D1_BIT - 8)  << 2))
                |    (12uL  << ((SD_D2_BIT - 8)  << 2))
                |    (12uL  << ((SD_D3_BIT - 8)  << 2))
                |    (12uL  << ((SD_CLK_BIT - 8) << 2));
  GPIOC_OSPEEDR &= ~((3uL   << (SD_D0_BIT  << 1)) |
                     (3uL   << (SD_D1_BIT  << 1))  |
                     (3uL   << (SD_D2_BIT  << 1))  |
                     (3uL   << (SD_D3_BIT  << 1))  |
                     (3uL   << (SD_CLK_BIT << 1)));
  GPIOC_OSPEEDR |=   (3uL   << (SD_D0_BIT  << 1)) |
                     (3uL   << (SD_D1_BIT  << 1)) |
                     (3uL   << (SD_D2_BIT  << 1)) |
                     (3uL   << (SD_D3_BIT  << 1)) |
                     (3uL   << (SD_CLK_BIT << 1));
  //
  // CMD line is also controlled by SDIO
  //
  GPIOD_MODER   &= ~(3uL   << (SD_CMD_BIT << 1));
  GPIOD_MODER   |=  (2uL   << (SD_CMD_BIT << 1));
  GPIOD_OTYPER  &= ~(1uL   <<  SD_CMD_BIT);
  GPIOD_PUPDR   &= ~(3uL   << (SD_CMD_BIT << 1));
  GPIOD_PUPDR   |=   1uL   << (SD_CMD_BIT << 1);    // The data transfer works reliably only if the pull-ups are enabled.
  GPIOD_AFRL    &= ~(0xFuL << (SD_CMD_BIT << 2));
  GPIOD_AFRL    |=  (12uL  << (SD_CMD_BIT << 2));
  GPIOD_OSPEEDR &= ~(3uL   << (SD_CMD_BIT << 1));
  GPIOD_OSPEEDR |=  (3uL   << (SD_CMD_BIT << 1));
  //
  // SDIO uses the stream 3, channel 4 of DMA2
  //
  DMA2_S3CR   &= ~S3CR_EN;          // Stop the data transfer
  while (DMA2_S3CR & S3CR_EN) {     // Wait for the stream to switch off
    ;
  }
  DMA2_LIFCR  |= LIFCR_CDMEIF3      // Clear any pending interrupts.
              |  LIFCR_CTEIF3
              |  LIFCR_CHTIF3
              |  LIFCR_CTCIF3
              |  LIFCR_CFEIF3
              ;
  DMA2_S3CR   = 0
              | S3CR_PSIZE_32BIT    // Peripheral bus width
              | S3CR_MSIZE_32BIT    // Memory bus width
              | S3CR_MINC           // Memory increment enable
              | S3CR_PRIO_HIGH      // Set priority to high
              | S3CR_CHSEL_CH4      // Channel connected to SDIO
              | S3CR_PFCTRL         // Peripheral controls the data transfer
              ;
  DMA2_S3FCR  = 0
              | S3FCR_FTH_FULL      // Full FIFO
              | S3FCR_DMDIS         // Disable direct mode (the only way the DMA transfer works together with SDIO)
              ;
  //
  // Compute the number of software loops to delay before writing to some SDIO registers.
  //
  Delay_ns  = (1000000000uL + ((FS_MMC_HW_CM_SDIO_CLK * 3) - 1)) / (FS_MMC_HW_CM_SDIO_CLK * 3);
  Delay_ns += (1000000000uL + ((FS_MMC_HW_CM_PCLK2 * 2) - 1)) / (FS_MMC_HW_CM_PCLK2 * 2);
  Inst_ns   = (1000000000uL + (FS_MMC_HW_CM_HCLK - 1)) / FS_MMC_HW_CM_HCLK;
  _NumLoopsWriteRegDelay = Delay_ns / Inst_ns;
  //
  // Initialize SDIO
  //
  SDIO_POWER  = 0;                  // Power off
  SDIO_CLKCR  = 0;                  // Disable the clock (Note 1)
  SDIO_ARG    = 0;
  SDIO_CMD    = 0;
  SDIO_DTIMER = 0;
  SDIO_DLEN   = 0;
  SDIO_DCTRL  = 0;
  SDIO_ICR    = 0x00C007FF;         // Clear interrupts
  SDIO_MASK   = 0;
  _WriteRegDelayed(&SDIO_POWER, 3); // Make sure that SDIO_POWER is written after the specified delay.
#if FS_MMC_HW_CM_USE_OS
  //
  // Unmask the interrupt sources.
  //
  SDIO_MASK   = MASK_ALL;
  //
  // Set the priority and enable the interrupts.
  //
  NVIC_SetPriority(DMA1_Channel3_IRQn, DMA_PRIO);
  NVIC_SetPriority(SDIO_IRQn, SDIO_PRIO);
  NVIC_EnableIRQ(DMA1_Channel3_IRQn);
  NVIC_EnableIRQ(SDIO_IRQn);
#endif // FS_MMC_HW_CM_USE_OS
  //
  // Wait for the power supply of the SD card to stabilize.
  //
  ms = FS_MMC_HW_CM_POWER_GOOD_DELAY;
  while (ms--) {
    _Delay1ms();
  }
}

/*********************************************************************
*
*       _HW_Delay
*
*  Function description
*    Blocks the code execution for the specified time.
*
*  Parameters
*    ms   Number of milliseconds to delay.
*/
static void _HW_Delay(int ms) {
  while (ms--) {
    _Delay1ms();
  }
}

/*********************************************************************
*
*       _HW_IsPresent
*
*  Function description
*    Returns the state of the media.
*
*  Parameters
*    Unit     Index of the SD / MMC host controller (0-based).
*
*  Return value
*    FS_MEDIA_STATE_UNKNOWN     The state of the storage device is not known.
*    FS_MEDIA_NOT_PRESENT       The storage device is not present.
*    FS_MEDIA_IS_PRESENT        The storage device is present.
*
*  Additional information
*    If the state is unknown, the function has to FS_MEDIA_STATE_UNKNOWN
*    and the higher layers of the file system will try to figure out
*    if a storage device is present or not.
*/
static int _HW_IsPresent(U8 Unit) {
  FS_USE_PARA(Unit);
  //
  // The card detect signal is not connected to the CPU on the evaluation board.
  //
  return FS_MEDIA_STATE_UNKNOWN;
}

/*********************************************************************
*
*       _HW_IsWriteProtected
*
*  Function description
*    Returns whether card is write protected or not.
*
*  Parameters
*    Unit     Index of the SD / MMC host controller (0-based).
*
*  Return value
*    ==0      The SD / MMC card is writable.
*    ==1      The SD / MMC card is write protected.
*/
static int _HW_IsWriteProtected(U8 Unit) {
  FS_USE_PARA(Unit);
  return 0;
}

/*********************************************************************
*
*       _HW_SetMaxSpeed
*
*  Function description
*    Configures the frequency of the clock supplied to SD / MMC card.
*
*  Parameters
*    Unit     Index of the SD / MMC host controller (0-based).
*    Freq     Requested clock frequency in kHz.
*
*  Return value
*    !=0  OK, actual configured clock frequency in kHz.
*    ==0  An error occurred.
*
*  Additional information
*    This function is called two times:
*    1. During card initialization
*       Initialize the frequency to not more than 400 kHz.
*    2. After card initialization
*       The CSD register of card is read and the max frequency
*       the card can operate is determined.
*       [In most cases: MMC cards 20 MHz, SD cards 25 MHz]
*    The configured clock frequency has to be smaller than or equal to
*    the frequency specified via Freq.
*
*  Notes
*    (1) Some IAR ARM versions will optimize out the multiplication of Fact & Freq.
*        This seems to affect all versions after 7.40. To be on the safe side the
*        optimization is disabled for all versions for now. The function is normally
*        only called twice during the initialization, therefor there is no performance penalty.
*/
#ifdef __ICCARM__
  #pragma optimize=none         // Note 1
#endif
static U16 _HW_SetMaxSpeed(U8 Unit, U16 Freq) {
  U32      Fact;
  unsigned Div;
  U32      ClkBypass;
  U32      v;

  FS_USE_PARA(Unit);
  if (Freq > FS_MMC_HW_CM_MAX_SD_CLK) {
    Freq = FS_MMC_HW_CM_MAX_SD_CLK;
  }
  _WaitForInactive();
  SDIO_CLKCR &= ~(CLKCR_CLKEN | 0xFFuL | CLKCR_CLK_BYPASS);
  Fact        = 2;
  Div         = 0;
  ClkBypass   = CLKCR_CLK_BYPASS;
  if (Freq < FS_MMC_HW_CM_SDIO_CLK) {
    ClkBypass = 0;
    while ((Freq * Fact) < FS_MMC_HW_CM_SDIO_CLK) {
      ++Fact;
      if (0xFF == ++Div) {
        break;
      }
    }
  }
  v = SDIO_CLKCR | Div | CLKCR_CLKEN | CLKCR_CLK_PWRSAV | ClkBypass;
  _WriteRegDelayed(&SDIO_CLKCR, v);
  Freq = FS_MMC_HW_CM_SDIO_CLK;
  if (ClkBypass == 0) {
    Freq /= (Div + 2);
  }
  return Freq;
}

/*********************************************************************
*
*       _HW_SetResponseTimeOut
*
*  Function description
*    Sets the maximum time the SD / MMC host controller
*    has to wait for a response.
*
*  Parameters
*    Unit     Index of the SD / MMC host controller (0-based).
*    Value    Number of SD / MMC clock cycles.
*/
static void _HW_SetResponseTimeOut(U8 Unit, U32 Value) {
  FS_USE_PARA(Unit);
  FS_USE_PARA(Value);
  //
  // The response timeout is fixed in hardware
  //
}

/*********************************************************************
*
*       _HW_SetReadDataTimeOut
*
*  Function description
*    Sets the maximum time the SD / MMC host controller has to wait
*    for the arrival of data.
*
*  Parameters
*    Unit     Index of the SD / MMC host controller (0-based).
*    Value    Number of SD / MMC clock cycles.
*/
static void _HW_SetReadDataTimeOut(U8 Unit, U32 Value) {
  FS_USE_PARA(Unit);
  SDIO_DTIMER = Value;
}

/*********************************************************************
*
*       _HW_SendCmd
*
*  Function description
*    Sends a command to the card.
*
*  Parameters
*    Unit           Index of the SD / MMC host controller (0-based).
*    Cmd            Command number according to [1]
*    CmdFlags       Additional information about the command to execute
*    ResponseType   Type of response as defined in [1]
*    Arg            Command parameter
*/
static void _HW_SendCmd(U8 Unit, unsigned Cmd, unsigned CmdFlags, unsigned ResponseType, U32 Arg) {
  U32 CmdCfg;
  U8  Direction;
  U32 NumBytes;
  U32 Status;
  U32 v;
#if (FS_MMC_HW_CM_USE_OS == 0)
  U32 TimeOut;
#endif

  FS_USE_PARA(Unit);
  _WaitForInactive();
#if (FS_VERSION > 40405)
  _RepeatSame = 0;
  if (CmdFlags & FS_MMC_CMD_FLAG_WRITE_BURST_FILL) {
    _RepeatSame = 1;
  }
#endif
  _IsR1Busy = 0;
  if (CmdFlags & FS_MMC_CMD_FLAG_SETBUSY) {
    _IsR1Busy = 1;
  }
  CmdCfg = CMD_CPSMEN
         | CMD_ENDCMDCMPLT
         | CMD_NIEN
         ;
  _IgnoreCRC = 0;
  switch (ResponseType) {
  case FS_MMC_RESPONSE_FORMAT_R3:
    _IgnoreCRC = 1;
    //lint -fallthrough
  case FS_MMC_RESPONSE_FORMAT_R1:
    CmdCfg |= CMD_WAITRESP_SHORT;
    break;
  case FS_MMC_RESPONSE_FORMAT_R2:
    CmdCfg |= CMD_WAITRESP_LONG;
    break;
  default:
    break;
  }
  v  = SDIO_CLKCR;
  v &= ~CLKCR_WIDBUS_MASK;
  if (CmdFlags & FS_MMC_CMD_FLAG_USE_SD4MODE) {           // 4 bit mode?
    v |= CLKCR_WIDBUS_4BIT;
  } else {
    v &=~CLKCR_WIDBUS_MASK;
  }
  SDIO_CLKCR = v;
  v = 0;
  if (CmdFlags & FS_MMC_CMD_FLAG_DATATRANSFER) {
    SDIO_ICR = ICR_DCRCFAIL
             | ICR_DTIMEOUT
             | ICR_TXUNDERR
             | ICR_DATAEND
             | ICR_DBCKEND
             | ICR_STBITERR
             | ICR_RXOVERR
             ;
    NumBytes = _BlockSize * _NumBlocks;
    SDIO_DLEN = NumBytes;
    if (CmdFlags & FS_MMC_CMD_FLAG_WRITETRANSFER) {
      Direction = MEMORY_TO_PERIPHERAL;
    } else {
      Direction = PERIPHERAL_TO_MEMORY;
    }
    _StartDMATransfer((U32 *)_pBuffer, (U32 *)&SDIO_FIFO, Direction, NumBytes);
    if ((CmdFlags & FS_MMC_CMD_FLAG_WRITETRANSFER) == 0) {
      //
      // Data transfer starts when the data control register is updated.
      //
      v = 0u
        | _DataBlockSize
        | DCTRL_DTDIR
        | DCTRL_DMAEN
        | DCTRL_DTEN
        ;
    }
  }
  SDIO_DCTRL = v;
  //
  // Clear pending status flags
  //
  SDIO_ICR = ICR_CCRCFAIL
           | ICR_CTIMEOUT
           | ICR_CMDREND
           | ICR_CMDSENT
           ;
#if FS_MMC_HW_CM_USE_OS
  _StatusSDIO = 0;
#endif
  SDIO_ARG = Arg;
  SDIO_CMD = CmdCfg | (Cmd & CMD_CMD_MASK);
  if (CmdFlags & FS_MMC_CMD_FLAG_INITIALIZE)  {
#if (FS_MMC_HW_CM_USE_OS == 0)
    TimeOut = WAIT_TIMEOUT_CYCLES;
#endif
    for (;;) {
#if FS_MMC_HW_CM_USE_OS
      Status = _StatusSDIO;
#else
      Status = SDIO_STA;
#endif
      if (Status & (STA_CMDSENT | STA_CMDREND | STA_CCRCFAIL |  STA_CTIMEOUT)) {
        break;
      }
#if FS_MMC_HW_CM_USE_OS
      {
        int r;

        r = FS_X_OS_Wait(WAIT_TIMEOUT_MS);
        if (r != 0) {
          break;
        }
      }
#else
      if (--TimeOut == 0u) {
        break;
      }
#endif
    }
  }
}

/*********************************************************************
*
*       _HW_GetResponse
*
*  Function description
*    Waits for a response to be received.
*
*  Parameters
*    Unit     Index of the SD / MMC host controller (0-based).
*    pBuffer  User allocated buffer where the response is stored.
*    Size     Size of the buffer in bytes.
*
*  Return values
*    FS_MMC_CARD_NO_ERROR                 Success
*    FS_MMC_CARD_RESPONSE_CRC_ERROR       CRC error in response
*    FS_MMC_CARD_RESPONSE_TIMEOUT         No response received
*    FS_MMC_CARD_RESPONSE_GENERIC_ERROR   Any other error
*
*  Notes
*    (1) The response data has to be stored at byte offset 1 when
*        the SD host controller does not provide the first byte
*        of response message, that is the byte that includes the
*        start bit.
*/
static int _HW_GetResponse(U8 Unit, void * pBuffer, U32 Size) {
  U8           * p;
  int            NumBytes;
  volatile U32 * pReg;
  U32            Data32;
  U32            Status;
  U32            NumWords;
#if (FS_MMC_HW_CM_USE_OS == 0)
  U32            TimeOut;
#endif

  FS_USE_PARA(Unit);
  p        = (U8 *)pBuffer;
  NumBytes = (int)Size;
#if (FS_MMC_HW_CM_USE_OS == 0)
  TimeOut = WAIT_TIMEOUT_CYCLES;
#endif
  for (;;) {
#if FS_MMC_HW_CM_USE_OS
    Status = _StatusSDIO;
#else
    Status = SDIO_STA;
#endif
    if ((Status & (STA_CMDSENT | STA_CMDREND | STA_CCRCFAIL | STA_CTIMEOUT)) != 0u) {
      break;
    }
#if FS_MMC_HW_CM_USE_OS
    {
      int r;

      r = FS_X_OS_Wait(WAIT_TIMEOUT_MS);
      if (r != 0) {
        return FS_MMC_CARD_RESPONSE_TIMEOUT;
      }
    }
#else
    if (--TimeOut == 0u) {
      return FS_MMC_CARD_RESPONSE_TIMEOUT;
    }
#endif
  }
  if ((STA_CTIMEOUT & Status) != 0u) {
    return FS_MMC_CARD_RESPONSE_TIMEOUT;
  }
  if (((STA_CCRCFAIL & Status) != 0u) && (_IgnoreCRC == 0u)) {
    return FS_MMC_CARD_READ_CRC_ERROR;
  }
  if (((STA_CMDREND & Status) != 0u) || (_IgnoreCRC != 0u)) {
    *p++ = (U8)SDIO_RESPCMD;
    NumBytes--;
    pReg = (volatile U32 *)(&SDIO_RESP1);
    NumWords = (U32)NumBytes >> 2;
    if (NumWords != 0u) {
      do {
        Data32 = *pReg++;
        *p++ = (U8)(Data32 >> 24);
        *p++ = (U8)(Data32 >> 16);
        *p++ = (U8)(Data32 >> 8);
        *p++ = (U8)Data32;
        NumBytes -= 4u;
      } while (--NumWords);
    }
    if (NumBytes == 3u) {
      Data32 = *pReg;
      *p++ = (U8)(Data32 >> 24);
      *p++ = (U8)(Data32 >> 16);
      *p++ = (U8)(Data32 >> 8);
    }
    if (NumBytes == 2u) {
      Data32 = *pReg;
      *p++ = (U8)(Data32 >> 24);
      *p++ = (U8)(Data32 >> 16);
    }
    if (NumBytes == 1u) {
      Data32 = *pReg;
      *p++ = (U8)(Data32 >> 24);
    }
  }
  return FS_MMC_CARD_NO_ERROR;
}

/*********************************************************************
*
*       _HW_ReadData
*
*  Function description
*    Reads data from the card using the SD / MMC host controller.
*
*  Parameters
*    Unit       Index of the SD / MMC host controller (0-based).
*    pBuffer    [OUT] Data received from storage device.
*    NumBytes   Number of bytes in one read data block.
*    NumBlocks  Number of data blocks to be read.
*
*  Return values
*    FS_MMC_CARD_NO_ERROR             Success
*    FS_MMC_CARD_READ_CRC_ERROR       CRC error in received data
*    FS_MMC_CARD_READ_TIMEOUT         No data received
*    FS_MMC_CARD_READ_GENERIC_ERROR   Any other error
*
*  Additional information
*    The total number of bytes to be transferred is NumBytes * NumBlocks.
*
*  Notes
*    (1) The DMA FIFO error flag (DMA2_LISR.FEIF3) is not checked
*        in this function. This flag does not necessarily indicate an error.
*        When the DMA is not able to transfer the data fast enough,
*        the SDIO_STA.RXOVERR will be set by SDIO peripheral.
*/
static int _HW_ReadData(U8 Unit, void * pBuffer, unsigned NumBytes, unsigned NumBlocks) {
  U32 StatusSDIO;
  U32 StatusDMA;
  int r;
#if (FS_MMC_HW_CM_USE_OS == 0)
  U32 TimeOut;
#endif

  FS_USE_PARA(Unit);
  FS_USE_PARA(pBuffer);
  FS_USE_PARA(NumBytes);
  FS_USE_PARA(NumBlocks);
#if (FS_MMC_HW_CM_USE_OS == 0)
  TimeOut = WAIT_TIMEOUT_CYCLES;
#endif
  for (;;) {
#if FS_MMC_HW_CM_USE_OS
    StatusSDIO = _StatusSDIO;
    StatusDMA  = _StatusDMA;
#else
    StatusSDIO = SDIO_STA;
    StatusDMA  = DMA2_LISR;
#endif
    if ((StatusDMA & LISR_TEIF3) != 0u) {
      //
      // This error is typically reported if pBuffer points to a buffer located
      // in the core coupled memory (base address: 0x1000_0000).
      // The core coupled memory is not accessible to DMA.
      //
      r = FS_MMC_CARD_READ_GENERIC_ERROR;
      break;
    }
    if ((StatusSDIO & STA_DCRCFAIL) != 0u) {
      r = FS_MMC_CARD_READ_CRC_ERROR;
      break;
    }
    if ((StatusSDIO & STA_DTIMEOUT) != 0u) {
      r = FS_MMC_CARD_READ_TIMEOUT;
      break;
    }
    if ((StatusSDIO & STA_RXOVERR) != 0u) {
      r = FS_MMC_CARD_READ_GENERIC_ERROR;
      break;
    }
    if ((StatusSDIO & STA_STBITERR) != 0u) {
      r = FS_MMC_CARD_READ_GENERIC_ERROR;
      break;
    }
    if ((StatusDMA & LISR_TCIF3) != 0u) {
      r = FS_MMC_CARD_NO_ERROR;
      break;
    }
#if FS_MMC_HW_CM_USE_OS
    r = FS_X_OS_Wait(WAIT_TIMEOUT_MS);
    if (r != 0) {
      r = FS_MMC_CARD_READ_TIMEOUT;
      break;
    }
#else
    if (--TimeOut == 0u) {
      r = FS_MMC_CARD_READ_TIMEOUT;
      break;
    }
#endif
  }
  //
  // In case of an error, disable DMA and the data state machine in SDIO.
  //
  if (r != 0) {
    DMA2_S3CR &= ~S3CR_EN;                    // Stop the data transfer
    while ((DMA2_S3CR & S3CR_EN) != 0u) {     // Wait for the stream to switch off
      ;
    }
    SDIO_DCTRL &= ~DCTRL_DTEN;                // Cancel SDIO data transfer.
  }
#if FS_MMC_HW_CM_DIV_SD_CLK
  //
  // Restore the SD clock speed.
  //
  if (_RegValueCLKCR != 0u) {
    SDIO_CLKCR = _RegValueCLKCR;
  }
#endif  // FS_MMC_HW_CM_DIV_SD_CLK
  return r;
}

/*********************************************************************
*
*       _HW_WriteData
*
*  Function description
*    Writes the data to SD / MMC card using the SD / MMC host controller.
*
*  Parameters
*    Unit       Index of the SD / MMC host controller (0-based).
*    pBuffer    [OUT] Data to be sent to storage device.
*    NumBytes   Number of bytes in one written data block.
*    NumBlocks  Number of data blocks to be written.
*
*  Return values
*    FS_MMC_CARD_NO_ERROR      Success
*    FS_MMC_CARD_READ_TIMEOUT  No data received
*
*  Additional information
*    The total number of bytes to be transferred is NumBytes * NumBlocks.
*
*  Notes
*    (1) The DMA FIFO error flag (DMA2_LISR.FEIF3) is not checked
*        in this function. This flag does not necessarily indicate an error.
*        When the DMA is not able to transfer the data fast enough,
*        the SDIO_STA.TXUNDERR will be set by SDIO peripheral.
*/
static int _HW_WriteData(U8 Unit, const void * pBuffer, unsigned NumBytes, unsigned NumBlocks) {
  U32 StatusSDIO;
  U32 StatusDMA;
  int r;
#if (FS_MMC_HW_CM_USE_OS == 0)
  U32 TimeOut;
#endif

  FS_USE_PARA(Unit);
  FS_USE_PARA(pBuffer);
  FS_USE_PARA(NumBytes);
  FS_USE_PARA(NumBlocks);
#if (FS_MMC_HW_CM_USE_OS == 0)
  TimeOut = WAIT_TIMEOUT_CYCLES;
#endif
  //
  // Data transfer starts when the data control register is updated.
  //
  SDIO_DCTRL = 0u
             | _DataBlockSize
             | DCTRL_DTEN
             | DCTRL_DMAEN
             ;
  for (;;) {
#if FS_MMC_HW_CM_USE_OS
    StatusSDIO = _StatusSDIO;
    StatusDMA  = _StatusDMA;
#else
    StatusSDIO = SDIO_STA;
    StatusDMA  = DMA2_LISR;
#endif
    if ((StatusDMA & LISR_TEIF3) != 0u) {
      //
      // This error is typically reported if pBuffer points to a buffer located
      // in the core coupled memory (base address: 0x1000_0000).
      // The core coupled memory is not accessible to DMA.
      //
      r = FS_MMC_CARD_WRITE_GENERIC_ERROR;
      break;
    }
    if ((StatusSDIO & STA_DCRCFAIL) != 0u) {
      r = FS_MMC_CARD_WRITE_CRC_ERROR;
      break;
    }
    if ((StatusSDIO & STA_DTIMEOUT) != 0u) {
      r = FS_MMC_CARD_WRITE_GENERIC_ERROR;
      break;
    }
    if ((StatusSDIO & STA_TXUNDERR) != 0u) {
      r = FS_MMC_CARD_WRITE_GENERIC_ERROR;
      break;
    }
    if ((StatusSDIO & STA_STBITERR) != 0u) {
      r = FS_MMC_CARD_WRITE_GENERIC_ERROR;
      break;
    }
    if ((StatusSDIO & STA_DATAEND) != 0u) {
      r = FS_MMC_CARD_NO_ERROR;
      break;
    }
#if FS_MMC_HW_CM_USE_OS
    r = FS_X_OS_Wait(WAIT_TIMEOUT_MS);
    if (r != 0) {
      r = FS_MMC_CARD_WRITE_GENERIC_ERROR;
      break;
    }
#else
    if (--TimeOut == 0u) {
      r = FS_MMC_CARD_WRITE_GENERIC_ERROR;
      break;
    }
#endif
  }
  //
  // In case of an error, disable DMA and the data state machine in SDIO.
  //
  if (r != 0) {
    DMA2_S3CR &= ~S3CR_EN;                    // Stop the data transfer
    while ((DMA2_S3CR & S3CR_EN) != 0u) {     // Wait for the stream to switch off
      ;
    }
    SDIO_DCTRL &= ~DCTRL_DTEN;                // Cancel SDIO data transfer.
  }
#if FS_MMC_HW_CM_DIV_SD_CLK
  //
  // Restore the SD clock speed.
  //
  if (_RegValueCLKCR != 0u) {
    SDIO_CLKCR = _RegValueCLKCR;
  }
#endif  // FS_MMC_HW_CM_DIV_SD_CLK
  return r;
}

/*********************************************************************
*
*       _HW_SetDataPointer
*
*  Function description
*    Tells the hardware layer where to read data from
*    or write data to.
*
*  Parameters
*    Unit     Index of the SD / MMC host controller (0-based).
*    p        Data buffer.
*
*  Additional information
*    This function is optional. It is required to be implemented for
*    some SD / MMC host controllers that need to know the address of
*    the data before sending the command to the card,
*    eg. when transferring the data via DMA.
*/
static void _HW_SetDataPointer(U8 Unit, const void * p) {
  FS_USE_PARA(Unit);
  _pBuffer = (void *)p; // cast const away as this buffer is used also for storing the data from card
}

/*********************************************************************
*
*       _HW_SetBlockLen
*
*  Function description
*    Sets the block size (sector size) that has to be transferred.
*
*  Parameters
*    Unit     Index of the SD / MMC host controller (0-based).
*/
static void _HW_SetBlockLen(U8 Unit, U16 BlockSize) {
  FS_USE_PARA(Unit);
  _BlockSize     = BlockSize;
  _DataBlockSize = _ld(BlockSize) << DCTRL_DBLOCKSIZE_SHIFT;
}

/*********************************************************************
*
*       _HW_SetNumBlocks
*
*  Function description
*    Sets the number of blocks (sectors) to be transferred.
*
*  Parameters
*    Unit     Index of the SD / MMC host controller (0-based).
*/
static void _HW_SetNumBlocks(U8 Unit, U16 NumBlocks) {
  FS_USE_PARA(Unit);
  _NumBlocks = NumBlocks;
}

/*********************************************************************
*
*       _HW_GetMaxReadBurst
*
*  Function description
*    Returns the number of block (sectors) that can be read at once
*    with a single READ_MULTIPLE_SECTOR command.
*
*  Parameters
*    Unit     Index of the SD / MMC host controller (0-based).
*
*  Return value
*    Number of sectors that can be read at once.
*/
static U16 _HW_GetMaxReadBurst(U8 Unit) {
  FS_USE_PARA(Unit);
  return (U16)MAX_NUM_BLOCKS;
}

/*********************************************************************
*
*       _HW_GetMaxWriteBurst
*
*  Function description
*    Returns the number of block (sectors) that can be written at once
*    with a single WRITE_MULTIPLE_SECTOR command.
*
*  Parameters
*    Unit     Index of the SD / MMC host controller (0-based).
*
*  Return value
*    Number of sectors that can be written at once.
*/
static U16 _HW_GetMaxWriteBurst(U8 Unit) {
  FS_USE_PARA(Unit);
  return (U16)MAX_NUM_BLOCKS;
}

#if (FS_VERSION > 40405)

  /*********************************************************************
*
*       _HW_GetMaxWriteBurstFill
*
*  Function description
*    Returns the number of block (sectors) that can be written at once
*    with a single WRITE_MULTIPLE_SECTOR command. The contents of the
*    sectors is filled with the same 32-bit pattern.
*
*  Parameters
*    Unit     Index of the SD / MMC host controller (0-based).
*
*  Return value
*    Number of sectors that can be written at once. The function has
*    to return 0 if the feature is not supported.
*/
static U16 _HW_GetMaxWriteBurstFill(U8 Unit) {
  FS_USE_PARA(Unit);
  return (U16)MAX_NUM_BLOCKS;
}

#endif

/*********************************************************************
*
*       Public data
*
**********************************************************************
*/
const FS_MMC_HW_TYPE_CM FS_MMC_HW_CM_GD32F450_GD_GD32450i_EVAL = {
  _HW_Init,
  _HW_Delay,
  _HW_IsPresent,
  _HW_IsWriteProtected,
  _HW_SetMaxSpeed,
  _HW_SetResponseTimeOut,
  _HW_SetReadDataTimeOut,
  _HW_SendCmd,
  _HW_GetResponse,
  _HW_ReadData,
  _HW_WriteData,
  _HW_SetDataPointer,
  _HW_SetBlockLen,
  _HW_SetNumBlocks,
  _HW_GetMaxReadBurst,
  _HW_GetMaxWriteBurst
#if (FS_VERSION > 40405)
  , NULL
  , _HW_GetMaxWriteBurstFill
#endif
#if (FS_VERSION > 50200)
  , NULL
  , NULL
  , NULL
  , NULL
  , NULL
  , NULL
  , NULL
#endif
};

/*************************** End of file ****************************/

