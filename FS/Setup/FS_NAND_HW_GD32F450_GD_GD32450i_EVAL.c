/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
*                           www.segger.com                           *
**********************************************************************

-------------------------- END-OF-HEADER -----------------------------

File    : FS_NAND_HW_GD32F450_GD_GD32450i_EVAL.c
Purpose : NAND flash hardware layer for the GigaDevice GD32450i-EVAL evaluation board.
Literature:
  [1] Datasheet GD32F450xx ARM Cortex-M4 32-bit MCU
    (\\FILESERVER\Techinfo\Company\GigaDevice\MCUs\GD32F4\GD32F450xx_Datasheet_Rev1.1.pdf)
  [2] User manual GD32F4xx ARM Cortex-M4 32-bit MCU for GD32F405xx, GD32F407xx and GD32F450xx
    (\\FILESERVER\Techinfo\Company\GigaDevice\MCUs\GD32F4\GD32F4xx_User_Manual_EN_V1.2.pdf)
  [3] User Manual GD32450I-EVAL
    (\\FILESERVER\Techinfo\Company\GigaDevice\MCUs\GD32F4\EvalBoard\GD32450I-EVAL\GD32450I-EVAL_User_Manual-V1.0.pdf)
  [3] Datasheet 1Gb NAND FLASH HY27UF081G2A HY27UF161G2A
    (\\FILESERVER\Techinfo\Company\Hynix\NANDFlash\HY27UF(08_16)1G2A.pdf)
*/

/*********************************************************************
*
*       #include Section
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
#ifndef   FS_NAND_HW_CYCLES_PER_1MS
  #define FS_NAND_HW_CYCLES_PER_1MS     30000     // Number of software cycles required to generate a 1ms delay
#endif

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

/*********************************************************************
*
*       NAND flash address
*/
#define NAND_BASE_ADDR          0x70000000
#define NAND_DATA               (void *)(NAND_BASE_ADDR + 0x00000)
#define NAND_CMD                (void *)(NAND_BASE_ADDR + 0x10000)
#define NAND_ADDR               (void *)(NAND_BASE_ADDR + 0x20000)

/*********************************************************************
*
*       Flexible static memory controller
*/
#define EXMC_BASE_ADDR          0xA0000000uL
#define EXMC_PCR2               (*(volatile U32*)(EXMC_BASE_ADDR + 0x40 + 0x20 * (2 - 1)))
#define EXMC_PMEM2              (*(volatile U32*)(EXMC_BASE_ADDR + 0x48 + 0x20 * (2 - 1)))
#define EXMC_PATT2              (*(volatile U32*)(EXMC_BASE_ADDR + 0x4C + 0x20 * (2 - 1)))

/*********************************************************************
*
*       Reset and clock control
*/
#define RCU_BASE_ADDR           0x40023800uL
#define RCU_AHB1ENR             (*(volatile U32*)(RCU_BASE_ADDR + 0x30))
#define RCU_AHB3ENR             (*(volatile U32*)(RCU_BASE_ADDR + 0x38))

/*********************************************************************
*
*       Port D registers
*/
#define GPIOD_BASE_ADDR         0x40020C00uL
#define GPIOD_MODER             (*(volatile U32 *)(GPIOD_BASE_ADDR + 0x00))
#define GPIOD_OSPEEDR           (*(volatile U32 *)(GPIOD_BASE_ADDR + 0x08))
#define GPIOD_PUPDR             (*(volatile U32 *)(GPIOD_BASE_ADDR + 0x0C))
#define GPIOD_IDR               (*(volatile U32 *)(GPIOD_BASE_ADDR + 0x10))
#define GPIOD_ODR               (*(volatile U32 *)(GPIOD_BASE_ADDR + 0x14))
#define GPIOD_AFRL              (*(volatile U32 *)(GPIOD_BASE_ADDR + 0x20))
#define GPIOD_AFRH              (*(volatile U32 *)(GPIOD_BASE_ADDR + 0x24))

/*********************************************************************
*
*       Port E registers
*/
#define GPIOE_BASE_ADDR         0x40021000uL
#define GPIOE_MODER             (*(volatile U32 *)(GPIOE_BASE_ADDR + 0x00))
#define GPIOE_OSPEEDR           (*(volatile U32 *)(GPIOE_BASE_ADDR + 0x08))
#define GPIOE_PUPDR             (*(volatile U32 *)(GPIOE_BASE_ADDR + 0x0C))
#define GPIOE_AFRL              (*(volatile U32 *)(GPIOE_BASE_ADDR + 0x20))
#define GPIOE_AFRH              (*(volatile U32 *)(GPIOE_BASE_ADDR + 0x24))

/*********************************************************************
*
*       GPIO bit positions of NAND flash signals
*/
#define NAND_CLE                11  // Port D
#define NAND_ALE                12  // Port D
#define NAND_D0                 14  // Port D
#define NAND_D1                 15  // Port D
#define NAND_D2                 0   // Port D
#define NAND_D3                 1   // Port D
#define NAND_D4                 7   // Port E
#define NAND_D5                 8   // Port E
#define NAND_D6                 9   // Port E
#define NAND_D7                 10  // Port E
#define NAND_NOE                4   // Port D
#define NAND_NWE                5   // Port D
#define NAND_NWAIT              6   // Port D
#define NAND_NCE1               7   // Port D

/*********************************************************************
*
*       Masks for the peripheral enable bits
*/
#define AHB1ENR_GPIODEN         3
#define AHB1ENR_GPIOEEN         4
#define AHB3ENR_FSMCEN          0

/*********************************************************************
*
*       Defines for I/O port configuration
*/
#define GPIO_MODE_AF_PP         ((2uL << 2) | 3uL)
#define GPIO_MODE_IN_PU         ((2uL << 2) | 0uL)

/*********************************************************************
*
*       Timeout for the NAND operation
*/
#define WAIT_TIMEOUT_MS         1000
#define WAIT_TIMEOUT_CYCLES     (WAIT_TIMEOUT_MS * FS_NAND_HW_CYCLES_PER_1MS)

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static volatile void * _pCurrentNANDAddr;

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       _HW_EnableCE
*/
static void _HW_EnableCE(U8 Unit) {
  FS_USE_PARA(Unit);
  //
  // The CS signal is driven by FSMC.
  //
}

/*********************************************************************
*
*       _HW_DisableCE
*/
static void _HW_DisableCE(U8 Unit) {
  FS_USE_PARA(Unit);
  //
  // The CS signal is driven by FSMC.
  //
}

/*********************************************************************
*
*       FS_NAND_HW_X_SetData
*/
static void _HW_SetDataMode(U8 Unit) {
  FS_USE_PARA(Unit);
  //
  // CLE low, ALE low
  //
  _pCurrentNANDAddr = NAND_DATA;
}

/*********************************************************************
*
*       FS_NAND_HW_X_SetCmd
*/
static void _HW_SetCmdMode(U8 Unit) {
  FS_USE_PARA(Unit);
  //
  // CLE high, ALE low
  //
  _pCurrentNANDAddr = NAND_CMD;
}

/*********************************************************************
*
*       FS_NAND_HW_X_SetAddr
*/
static void _HW_SetAddrMode(U8 Unit) {
  FS_USE_PARA(Unit);
  //
  // CLE low, ALE high
  //
  _pCurrentNANDAddr = NAND_ADDR;
}

/*********************************************************************
*
*       _HW_Read_x8
*/
static void _HW_Read_x8(U8 Unit, void * pData, unsigned NumBytes) {
  FS_USE_PARA(Unit);
  FS_MEMCPY(pData, (void *)_pCurrentNANDAddr, NumBytes);
}

/*********************************************************************
*
*       _HW_Write_x8
*/
static void _HW_Write_x8(U8 Unit, const void * pData, unsigned NumBytes) {
  FS_USE_PARA(Unit);
  FS_MEMCPY((void *)_pCurrentNANDAddr, pData, NumBytes);
}

/*********************************************************************
*
*       _HW_Init_x8
*/
static void _HW_Init_x8(U8 Unit) {
  U32 Timings;

  FS_USE_PARA(Unit);
  //
  // Enable the clocks of peripherals.
  //
  RCU_AHB1ENR |= 0uL
              |  (1uL << AHB1ENR_GPIODEN)
              |  (1uL << AHB1ENR_GPIOEEN)
              ;
  RCU_AHB3ENR |=  1uL << AHB3ENR_FSMCEN;
  //
  // Configure the pins which drive the NAND flash signals.
  //
  GPIOD_MODER   &= ~(( 3uL << (NAND_CLE       << 1)) |
                     ( 3uL << (NAND_ALE       << 1)) |
                     ( 3uL << (NAND_D0        << 1)) |
                     ( 3uL << (NAND_D1        << 1)) |
                     ( 3uL << (NAND_D2        << 1)) |
                     ( 3uL << (NAND_D3        << 1)) |
                     ( 3uL << (NAND_NOE       << 1)) |
                     ( 3uL << (NAND_NWE       << 1)) |
                     ( 3uL << (NAND_NCE1      << 1)));
  GPIOD_MODER   |=   ( 2uL << (NAND_CLE       << 1))
                |    ( 2uL << (NAND_ALE       << 1))
                |    ( 2uL << (NAND_D0        << 1))
                |    ( 2uL << (NAND_D1        << 1))
                |    ( 2uL << (NAND_D2        << 1))
                |    ( 2uL << (NAND_D3        << 1))
                |    ( 2uL << (NAND_NOE       << 1))
                |    ( 2uL << (NAND_NWE       << 1))
                |    ( 2uL << (NAND_NCE1      << 1))
                ;
  GPIOD_PUPDR   &= ~(( 3uL << (NAND_CLE       << 1)) |
                     ( 3uL << (NAND_ALE       << 1)) |
                     ( 3uL << (NAND_D0        << 1)) |
                     ( 3uL << (NAND_D1        << 1)) |
                     ( 3uL << (NAND_D2        << 1)) |
                     ( 3uL << (NAND_D3        << 1)) |
                     ( 3uL << (NAND_NOE       << 1)) |
                     ( 3uL << (NAND_NWE       << 1)) |
                     ( 3uL << (NAND_NCE1      << 1)));
  GPIOD_AFRH    &= ~((15uL << ((NAND_CLE - 8) << 2)) |
                     (15uL << ((NAND_ALE - 8) << 2)) |
                     (15uL << ((NAND_D0  - 8) << 2)) |
                     (15uL << ((NAND_D1  - 8) << 2)));
  GPIOD_AFRH    |=   (12uL << ((NAND_CLE - 8) << 2))
                |    (12uL << ((NAND_ALE - 8) << 2))
                |    (12uL << ((NAND_D0  - 8) << 2))
                |    (12uL << ((NAND_D1  - 8) << 2));
  GPIOD_AFRL    &= ~((15uL << (NAND_D2        << 2)) |
                     (15uL << (NAND_D3        << 2)) |
                     (15uL << (NAND_NOE       << 2)) |
                     (15uL << (NAND_NWE       << 2)) |
                     (15uL << (NAND_NCE1      << 2)));
  GPIOD_AFRL    |=   (12uL << (NAND_D2        << 2))
                |    (12uL << (NAND_D3        << 2))
                |    (12uL << (NAND_NOE       << 2))
                |    (12uL << (NAND_NWE       << 2))
                |    (12uL << (NAND_NCE1      << 2));
  GPIOD_OSPEEDR &= ~(( 3uL << (NAND_CLE       << 1)) |
                     ( 3uL << (NAND_ALE       << 1)) |
                     ( 3uL << (NAND_D0        << 1)) |
                     ( 3uL << (NAND_D1        << 1)) |
                     ( 3uL << (NAND_D2        << 1)) |
                     ( 3uL << (NAND_D3        << 1)) |
                     ( 3uL << (NAND_NOE       << 1)) |
                     ( 3uL << (NAND_NWE       << 1)) |
                     ( 3uL << (NAND_NCE1      << 1)));
  GPIOD_OSPEEDR |=   ( 1uL << (NAND_CLE       << 1)) |
                     ( 1uL << (NAND_ALE       << 1)) |
                     ( 1uL << (NAND_D0        << 1)) |
                     ( 1uL << (NAND_D1        << 1)) |
                     ( 1uL << (NAND_D2        << 1)) |
                     ( 1uL << (NAND_D3        << 1)) |
                     ( 1uL << (NAND_NOE       << 1)) |
                     ( 1uL << (NAND_NWE       << 1)) |
                     ( 1uL << (NAND_NCE1      << 1));
  GPIOE_MODER   &= ~(( 3uL << (NAND_D4        << 1)) |
                     ( 3uL << (NAND_D5        << 1)) |
                     ( 3uL << (NAND_D6        << 1)) |
                     ( 3uL << (NAND_D7        << 1)));
  GPIOE_MODER   |=   ( 2uL << (NAND_D4        << 1))
                |    ( 2uL << (NAND_D5        << 1))
                |    ( 2uL << (NAND_D6        << 1))
                |    ( 2uL << (NAND_D7        << 1))
                ;
  GPIOE_PUPDR   &= ~(( 3uL << (NAND_D4        << 1)) |
                     ( 3uL << (NAND_D5        << 1)) |
                     ( 3uL << (NAND_D6        << 1)) |
                     ( 3uL << (NAND_D7        << 1)));
  GPIOE_AFRH    &= ~((15uL << ((NAND_D5 - 8)  << 2)) |
                     (15uL << ((NAND_D6 - 8)  << 2)) |
                     (15uL << ((NAND_D7 - 8)  << 2)));
  GPIOE_AFRH    |=   (12uL << ((NAND_D5 - 8)  << 2))
                |    (12uL << ((NAND_D6 - 8)  << 2))
                |    (12uL << ((NAND_D7 - 8)  << 2));
  GPIOE_AFRL    &= ~( 15uL << (NAND_D4        << 2));
  GPIOE_AFRL    |=    12uL << (NAND_D4        << 2);
  GPIOE_OSPEEDR &= ~(( 3uL << (NAND_D4        << 1)) |
                     ( 3uL << (NAND_D5        << 1)) |
                     ( 3uL << (NAND_D6        << 1)) |
                     ( 3uL << (NAND_D7        << 1)));
  GPIOE_OSPEEDR |=   ( 1uL << (NAND_D4        << 1)) |
                     ( 1uL << (NAND_D5        << 1)) |
                     ( 1uL << (NAND_D6        << 1)) |
                     ( 1uL << (NAND_D7        << 1));
  //
  // The busy signal is an input with pull-up read by the HW layer.
  //
  GPIOD_MODER &= ~(3uL   << (NAND_NWAIT << 1));
  GPIOD_PUPDR &= ~(3uL   << (NAND_NWAIT << 1));
  GPIOD_PUPDR |=   1uL   << (NAND_NWAIT << 1);
  GPIOD_AFRL  &= ~(0xFuL << (NAND_NWAIT << 2));
  //
  // Configure the timings of the memory controller. Note that these timings are used
  // for read as well as for write operations.
  //
  // SET time:  Time to wait after address has been set and before the NWE and NOE signals become active (LOW)
  //            Real delay is RegVal + 1 (Min is RegVal == 0, max. RegVal == 0xFF)
  // WAIT time: Minimum time the NWE and NOE signals stay active (LOW)
  //            Real delay is RegVal + 1 (Min is RegVal == 1, max. RegVal == 0xFF)
  // HOLD time: Time to keep the data valid on the bus after the NWE and NOE signals are deactivated (HIGH)
  //            Real delay is RegVal     (Min is RegVal == 1, max. RegVal == 0xFF)
  // HiZ time:  Time the databus is kept in HiZ after the start of a NAND Flash write access. This parameter is used only for write operations.
  //            Real delay is RegVal     (Min is RegVal == 0, max. RegVal == 0xFF)
  //
  // Delays are calculated in HCLKs which is the frequency supplied to the external memory controller.
  // The calculations below assume that HCLK is 200 MHz -> 5ns per clock and that the connected NAND flash is a Hynix HY27UF081G2A device.
  // These settings have to be updated if external memory controller is clocked at
  // a different (especially higher) frequency or a different NAND flash device is used.
  //
  // SET time is maximum of:
  //    tCS  - tWP = 25 ns - 15 ns = 10ns
  //    tCLS - tWP = 15 ns - 15 ns = 0ns
  //    tALS - tWP = 15 ns - 15 ns = 0ns
  //    -> 10 ns -> min 1 (1 clock cycle is added automatically by EXMC according to [2])
  //
  // WAIT time is the maximum of:
  //    tDH  = 5 ns
  //    tREH = 10 ns
  //    tWP  = 15 ns
  //    tRP  = 15 ns
  //    tREA = 25 ns
  //    -> 25 ns -> min 4 (1 clock cycle is added automatically by EXMC according to [2])
  //
  // HOLD time is the maximum of:
  //    tDS  = 5 ns
  //    tCLH = 5 ns
  //    tCH  = 5 ns
  //    tALH = 5 ns
  //    -> 5 ns -> min 1
  //
  // HiZ time is:
  //    tWP - tDS = 15 ns - 5 ns = 10 ns
  //    -> 10 ns -> min 2
  //
  Timings    = 0
             | (3uL << 24)  // HiZ
             | (2uL << 16)  // HOLD
             | (4uL << 8)   // WAIT
             | (2uL << 0)   // SET
             ;
  EXMC_PMEM2 = Timings;
  EXMC_PATT2 = Timings;
  EXMC_PCR2  = 0
             | (1uL << 3)   // Memory type: NAND flash
             | (1uL << 2)   // Enable memory bank
             ;
}

/*********************************************************************
*
*       _HW_Read_x16
*/
static void _HW_Read_x16(U8 Unit, void * pData, unsigned NumBytes) {
  FS_USE_PARA(Unit);
  FS_MEMCPY(pData, (void *)_pCurrentNANDAddr, NumBytes);
}

/*********************************************************************
*
*       _HW_Write_x16
*/
static void _HW_Write_x16(U8 Unit, const void * pData, unsigned NumBytes) {
  FS_USE_PARA(Unit);
  FS_MEMCPY((void *)_pCurrentNANDAddr, pData, NumBytes);
}

/*********************************************************************
*
*       _HW_Init_x16
*/
static void _HW_Init_x16(U8 Unit) {
  FS_USE_PARA(Unit);
  _HW_Init_x8(Unit);
  EXMC_PCR2 |= (1uL << 4)   // 16-bit wide bus
            ;
}

/*********************************************************************
*
*       _HW_WaitWhileBusy
*/
static int _HW_WaitWhileBusy(U8 Unit, unsigned us) {
  U32 Status;
  U32 TimeOut;

  FS_USE_PARA(Unit);
  FS_USE_PARA(us);
  TimeOut = WAIT_TIMEOUT_CYCLES;
  for (;;) {
    Status = GPIOD_IDR;
    if ((Status & (1uL << NAND_NWAIT)) != 0u) {
      return 0;                         // OK, the NAND flash is ready.
    }
    if (TimeOut-- == 0u) {
      return 1;                         // Error, the NAND flash is still busy.
    }
  }
}

/*********************************************************************
*
*       Public const
*
**********************************************************************
*/

/*********************************************************************
*
*       FS_NAND_HW_GD32F450_GD_GD32450i_EVAL
*/
const FS_NAND_HW_TYPE FS_NAND_HW_GD32F450_GD_GD32450i_EVAL = {
  _HW_Init_x8,
  _HW_Init_x16,
  _HW_DisableCE,
  _HW_EnableCE,
  _HW_SetAddrMode,
  _HW_SetCmdMode,
  _HW_SetDataMode,
  _HW_WaitWhileBusy,
  _HW_Read_x8,
  _HW_Write_x8,
  _HW_Read_x16,
  _HW_Write_x16
};

/**************************** end of file ***************************/

