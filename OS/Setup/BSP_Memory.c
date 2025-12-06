/*********************************************************************
*                     SEGGER Microcontroller GmbH                    *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 1995 - 2021 SEGGER Microcontroller GmbH                  *
*                                                                    *
*       Internet: segger.com  Support: support_embos@segger.com      *
*                                                                    *
**********************************************************************
*                                                                    *
*       embOS * Real time operating system for microcontrollers      *
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
*       OS version: V5.14.0.0                                        *
*                                                                    *
**********************************************************************

-------------------------- END-OF-HEADER -----------------------------
Purpose : External SDRAM initialization for the GD32F450I Eval.
Note    : JP17 needs to be set to 2-3 for SDRAM.
*/

#include "BSP.h"

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

/*********************************************************************
*
*       RCU
*/
#define RCU_BASEADDR          (0x40023800u)
#define RCU_AHB1ENR           (*(volatile unsigned int*)(RCU_BASEADDR + 0x30u))
#define RCU_AHB3ENR           (*(volatile unsigned int*)(RCU_BASEADDR + 0x38u))

/*********************************************************************
*
*       EXMC
*/
#define EXMC_BASEADDR         (0xA0000000u)
#define EXMC_SDCTL0           (*(volatile unsigned int*)(EXMC_BASEADDR + 0x140u))
#define EXMC_SDTCFG0          (*(volatile unsigned int*)(EXMC_BASEADDR + 0x148u))
#define EXMC_SDCMD            (*(volatile unsigned int*)(EXMC_BASEADDR + 0x150u))
#define EXMC_SDARI            (*(volatile unsigned int*)(EXMC_BASEADDR + 0x154u))
#define EXMC_SDSTAT           (*(volatile unsigned int*)(EXMC_BASEADDR + 0x158u))

/*********************************************************************
*
*       GPIOs
*/
#define GPIOB_BASEADDR        (0x40020400u)
#define GPIOC_BASEADDR        (0x40020800u)
#define GPIOD_BASEADDR        (0x40020C00u)
#define GPIOE_BASEADDR        (0x40021000u)
#define GPIOF_BASEADDR        (0x40021400u)
#define GPIOG_BASEADDR        (0x40021800u)
#define GPIOH_BASEADDR        (0x40021C00u)
#define GPIOI_BASEADDR        (0x40022000u)

#define GPIOx_CTL(x)          (*(volatile unsigned int*)((x) + 0x00u))
#define GPIOx_OMODE(x)        (*(volatile unsigned int*)((x) + 0x04u))
#define GPIOx_OSPD(x)         (*(volatile unsigned int*)((x) + 0x08u))
#define GPIOx_PUD(x)          (*(volatile unsigned int*)((x) + 0x0Cu))
#define GPIOx_ISTAT(x)        (*(volatile unsigned int*)((x) + 0x10u))
#define GPIOx_OCTL(x)         (*(volatile unsigned int*)((x) + 0x14u))
#define GPIOx_BOP(x)          (*(volatile unsigned int*)((x) + 0x18u))
#define GPIOx_LOCK(x)         (*(volatile unsigned int*)((x) + 0x1Cu))
#define GPIOx_AFSEL0(x)       (*(volatile unsigned int*)((x) + 0x20u))
#define GPIOx_AFSEL1(x)       (*(volatile unsigned int*)((x) + 0x24u))

#define GPIO_PIN_0            ( 0)
#define GPIO_PIN_1            ( 1)
#define GPIO_PIN_2            ( 2)
#define GPIO_PIN_3            ( 3)
#define GPIO_PIN_4            ( 4)
#define GPIO_PIN_5            ( 5)
#define GPIO_PIN_6            ( 6)
#define GPIO_PIN_7            ( 7)
#define GPIO_PIN_8            ( 8)
#define GPIO_PIN_9            ( 9)
#define GPIO_PIN_10           (10)
#define GPIO_PIN_11           (11)
#define GPIO_PIN_12           (12)
#define GPIO_PIN_13           (13)
#define GPIO_PIN_14           (14)
#define GPIO_PIN_15           (15)

#define GPIOA_BitPos          (0)
#define GPIOB_BitPos          (1)
#define GPIOC_BitPos          (2)
#define GPIOD_BitPos          (3)
#define GPIOE_BitPos          (4)
#define GPIOF_BitPos          (5)
#define GPIOG_BitPos          (6)
#define GPIOH_BitPos          (7)
#define GPIOI_BitPos          (8)

#define CMD_CLK_ENABLE        ( 1u)
#define CMD_PALL              ( 2u)
#define CMD_AUTOREFRESH_MODE  ( 3u)
#define CMD_LOAD_MODE         ( 4u)
#define CMD_TARGET_BANK1      (16u)
#define CMD_TARGET_BANK2      ( 8u)
#define CMD_AUTOREFRESH_1     ( 1u)
#define CMD_AUTOREFRESH_4     ( 4u)
#define CMD_AUTOREFRESH_8     ( 8u)

#define REFRESH_COUNT         (761u)  // SDRAM refresh counter

/*********************************************************************
*
*       Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _PinInit()
*/
static void _PinInit(void) {
  //
  // Enable GPIOs clock
  //
  RCU_AHB1ENR = RCU_AHB1ENR
              | (1u << GPIOB_BitPos)
              | (1u << GPIOC_BitPos)
              | (1u << GPIOD_BitPos)
              | (1u << GPIOE_BitPos)
              | (1u << GPIOF_BitPos)
              | (1u << GPIOG_BitPos)
              | (1u << GPIOH_BitPos)
              ;
  //
  // GPIOC Init
  // Pin 2, 5
  //
  GPIOx_AFSEL0(GPIOC_BASEADDR) |= (0xCu << (GPIO_PIN_2 * 4))                  // PC2, alternate function 12
                               |  (0xCu << (GPIO_PIN_5 * 4))                  // PC5, alternate function 12
                               ;
  GPIOx_CTL(GPIOC_BASEADDR)    |= (0x2u << (GPIO_PIN_2 * 2))                  // PB2, alternate function mode
                               |  (0x2u << (GPIO_PIN_5 * 2))                  // PB5, alternate function mode
                               ;
  GPIOx_OSPD(GPIOC_BASEADDR)   |= (0x2u << (GPIO_PIN_2 * 2))                  // PB2, fast speed
                               |  (0x2u << (GPIO_PIN_5 * 2))                  // PB5, fast speed
                               ;
  GPIOx_PUD(GPIOC_BASEADDR)    |= (0x1u << (GPIO_PIN_2 * 2))                  // PB0, pull-up
                               |  (0x1u << (GPIO_PIN_5 * 2))                  // PB1, pull-up
                               ;
  //
  // GPIOD Init
  // Pin 0,1,8,9,10,14,15
  //
  GPIOx_AFSEL0(GPIOD_BASEADDR) |= (0xCu << (GPIO_PIN_0 * 4))                  // PB0, alternate function 12
                               |  (0xCu << (GPIO_PIN_1 * 4))                  // PB1, alternate function 12
                               ;
  GPIOx_AFSEL1(GPIOD_BASEADDR) |= (0xCu << ((GPIO_PIN_8  - GPIO_PIN_8) * 4))  // PB8, alternate function 12
                               |  (0xCu << ((GPIO_PIN_9  - GPIO_PIN_8) * 4))  // PB9, alternate function 12
                               |  (0xCu << ((GPIO_PIN_10 - GPIO_PIN_8) * 4))  // PB10, alternate function 12
                               |  (0xCu << ((GPIO_PIN_14 - GPIO_PIN_8) * 4))  // PB14, alternate function 12
                               |  (0xCu << ((GPIO_PIN_15 - GPIO_PIN_8) * 4))  // PB15, alternate function 12
                               ;
  GPIOx_CTL(GPIOD_BASEADDR)    |= (0x2u << (GPIO_PIN_0  * 2))                 // PB0, alternate function mode
                               |  (0x2u << (GPIO_PIN_1  * 2))                 // PB1, alternate function mode
                               |  (0x2u << (GPIO_PIN_8  * 2))                 // PB8, alternate function mode
                               |  (0x2u << (GPIO_PIN_9  * 2))                 // PB9, alternate function mode
                               |  (0x2u << (GPIO_PIN_10 * 2))                 // PB10, alternate function mode
                               |  (0x2u << (GPIO_PIN_14 * 2))                 // PB14, alternate function mode
                               |  (0x2u << (GPIO_PIN_15 * 2))                 // PB15, alternate function mode
                               ;
  GPIOx_OSPD(GPIOD_BASEADDR)   |= (0x2u << (GPIO_PIN_0  * 2))                 // PB0, fast speed
                               |  (0x2u << (GPIO_PIN_1  * 2))                 // PB1, fast speed
                               |  (0x2u << (GPIO_PIN_8  * 2))                 // PB8, fast speed
                               |  (0x2u << (GPIO_PIN_9  * 2))                 // PB9, fast speed
                               |  (0x2u << (GPIO_PIN_10 * 2))                 // PB10, fast speed
                               |  (0x2u << (GPIO_PIN_14 * 2))                 // PB14, fast speed
                               |  (0x2u << (GPIO_PIN_15 * 2))                 // PB15, fast speed
                               ;
// set Pullup with GPIOx_PUD
  GPIOx_PUD(GPIOD_BASEADDR)    |= (0x1u << (GPIO_PIN_0  * 2))                 // PB0, fast speed
                               |  (0x1u << (GPIO_PIN_1  * 2))                 // PB1, fast speed
                               |  (0x1u << (GPIO_PIN_8  * 2))                 // PB8, fast speed
                               |  (0x1u << (GPIO_PIN_9  * 2))                 // PB9, fast speed
                               |  (0x1u << (GPIO_PIN_10 * 2))                 // PB10, fast speed
                               |  (0x1u << (GPIO_PIN_14 * 2))                 // PB14, fast speed
                               |  (0x1u << (GPIO_PIN_15 * 2))                 // PB15, fast speed
                               ;
  //
  // GPIOE Init
  // Pin 0,1,7,8,9,10,11,12,13,14,15
  //
  GPIOx_AFSEL0(GPIOE_BASEADDR) |= (0xCu << ((GPIO_PIN_0) * 4))                // PB0, alternate function 12
                               |  (0xCu << ((GPIO_PIN_1) * 4))                // PB1, alternate function 12
                               |  (0xCu << ((GPIO_PIN_7) * 4))                // PB7, alternate function 12
                               ;
  GPIOx_AFSEL1(GPIOE_BASEADDR) |= (0xCu << ((GPIO_PIN_8  - GPIO_PIN_8) * 4))  // PB8,  alternate function 12
                               |  (0xCu << ((GPIO_PIN_9  - GPIO_PIN_8) * 4))  // PB9,  alternate function 12
                               |  (0xCu << ((GPIO_PIN_10 - GPIO_PIN_8) * 4))  // PB10, alternate function 12
                               |  (0xCu << ((GPIO_PIN_11 - GPIO_PIN_8) * 4))  // PB11, alternate function 12
                               |  (0xCu << ((GPIO_PIN_12 - GPIO_PIN_8) * 4))  // PB12, alternate function 12
                               |  (0xCu << ((GPIO_PIN_13 - GPIO_PIN_8) * 4))  // PB13, alternate function 12
                               |  (0xCu << ((GPIO_PIN_14 - GPIO_PIN_8) * 4))  // PB14, alternate function 12
                               |  (0xCu << ((GPIO_PIN_15 - GPIO_PIN_8) * 4))  // PB15, alternate function 12
                               ;
  GPIOx_CTL(GPIOE_BASEADDR)    |= (0x2u << (GPIO_PIN_0  * 2))                 // PB0, alternate function mode
                               |  (0x2u << (GPIO_PIN_1  * 2))                 // PB1, alternate function mode
                               |  (0x2u << (GPIO_PIN_7  * 2))                 // PB7, alternate function mode
                               |  (0x2u << (GPIO_PIN_8  * 2))                 // PB8, alternate function mode
                               |  (0x2u << (GPIO_PIN_9  * 2))                 // PB9, alternate function mode
                               |  (0x2u << (GPIO_PIN_10 * 2))                 // PB10, alternate function mode
                               |  (0x2u << (GPIO_PIN_11 * 2))                 // PB11, alternate function mode
                               |  (0x2u << (GPIO_PIN_12 * 2))                 // PB12, alternate function mode
                               |  (0x2u << (GPIO_PIN_13 * 2))                 // PB13, alternate function mode
                               |  (0x2u << (GPIO_PIN_14 * 2))                 // PB14, alternate function mode
                               |  (0x2u << (GPIO_PIN_15 * 2))                 // PB15, alternate function mode
                               ;
  GPIOx_OSPD(GPIOE_BASEADDR)   |= (0x2u << (GPIO_PIN_0  * 2))                 // PB0, fast speed
                               |  (0x2u << (GPIO_PIN_1  * 2))                 // PB1, fast speed
                               |  (0x2u << (GPIO_PIN_7  * 2))                 // PB7, fast speed
                               |  (0x2u << (GPIO_PIN_8  * 2))                 // PB8, fast speed
                               |  (0x2u << (GPIO_PIN_9  * 2))                 // PB9, fast speed
                               |  (0x2u << (GPIO_PIN_10 * 2))                 // PB10, fast speed
                               |  (0x2u << (GPIO_PIN_11 * 2))                 // PB11, fast speed
                               |  (0x2u << (GPIO_PIN_12 * 2))                 // PB12, fast speed
                               |  (0x2u << (GPIO_PIN_13 * 2))                 // PB13, fast speed
                               |  (0x2u << (GPIO_PIN_14 * 2))                 // PB14, fast speed
                               |  (0x2u << (GPIO_PIN_15 * 2))                 // PB15, fast speed
                               ;
  GPIOx_PUD(GPIOE_BASEADDR)    |= (0x1u << (GPIO_PIN_0  * 2))                 // pull-up
                               |  (0x1u << (GPIO_PIN_1  * 2))                 // pull-up
                               |  (0x1u << (GPIO_PIN_7  * 2))                 // pull-up
                               |  (0x1u << (GPIO_PIN_8  * 2))                 // pull-up
                               |  (0x1u << (GPIO_PIN_9  * 2))                 // pull-up
                               |  (0x1u << (GPIO_PIN_10 * 2))                 // pull-up
                               |  (0x1u << (GPIO_PIN_11 * 2))                 // pull-up
                               |  (0x1u << (GPIO_PIN_12 * 2))                 // pull-up
                               |  (0x1u << (GPIO_PIN_13 * 2))                 // pull-up
                               |  (0x1u << (GPIO_PIN_14 * 2))                 // pull-up
                               |  (0x1u << (GPIO_PIN_15 * 2))                 // pull-up
                               ;
  //
  // GPIOF Init
  // Pin 0,1,2,3,4,5,11,12,13,14,15
  // Alternate function 12
  //
  GPIOx_AFSEL0(GPIOF_BASEADDR) |= (0xCu << (GPIO_PIN_0 * 4))
                               |  (0xCu << (GPIO_PIN_1 * 4))
                               |  (0xCu << (GPIO_PIN_2 * 4))
                               |  (0xCu << (GPIO_PIN_3 * 4))
                               |  (0xCu << (GPIO_PIN_4 * 4))
                               |  (0xCu << (GPIO_PIN_5 * 4))
                               ;
  GPIOx_AFSEL1(GPIOF_BASEADDR) |= (0xCu << ((GPIO_PIN_11 - GPIO_PIN_8) * 4))
                               |  (0xCu << ((GPIO_PIN_12 - GPIO_PIN_8) * 4))
                               |  (0xCu << ((GPIO_PIN_13 - GPIO_PIN_8) * 4))
                               |  (0xCu << ((GPIO_PIN_14 - GPIO_PIN_8) * 4))
                               |  (0xCu << ((GPIO_PIN_15 - GPIO_PIN_8) * 4))
                               ;
  //
  // Set Alternate Function Push Pull Mode (0x02)
  //
  GPIOx_CTL(GPIOF_BASEADDR)    |= (0x2u << (GPIO_PIN_0  * 2))
                               |  (0x2u << (GPIO_PIN_1  * 2))
                               |  (0x2u << (GPIO_PIN_2  * 2))
                               |  (0x2u << (GPIO_PIN_3  * 2))
                               |  (0x2u << (GPIO_PIN_4  * 2))
                               |  (0x2u << (GPIO_PIN_5  * 2))
                               |  (0x2u << (GPIO_PIN_11 * 2))
                               |  (0x2u << (GPIO_PIN_12 * 2))
                               |  (0x2u << (GPIO_PIN_13 * 2))
                               |  (0x2u << (GPIO_PIN_14 * 2))
                               |  (0x2u << (GPIO_PIN_15 * 2))
                               ;
  //
  // set GPIO_SPEED to "fast speed" = 0x02
  //
  GPIOx_OSPD(GPIOF_BASEADDR)   |= (0x2u << (GPIO_PIN_0  * 2))
                               |  (0x2u << (GPIO_PIN_1  * 2))
                               |  (0x2u << (GPIO_PIN_2  * 2))
                               |  (0x2u << (GPIO_PIN_3  * 2))
                               |  (0x2u << (GPIO_PIN_4  * 2))
                               |  (0x2u << (GPIO_PIN_5  * 2))
                               |  (0x2u << (GPIO_PIN_11 * 2))
                               |  (0x2u << (GPIO_PIN_12 * 2))
                               |  (0x2u << (GPIO_PIN_13 * 2))
                               |  (0x2u << (GPIO_PIN_14 * 2))
                               |  (0x2u << (GPIO_PIN_15 * 2))
                               ;
  //
  // set GPIO_PULLUP("Pull-up activation") (0x01)
  //
  GPIOx_PUD(GPIOF_BASEADDR)    |= (0x1u << (GPIO_PIN_0  * 2))
                               |  (0x1u << (GPIO_PIN_1  * 2))
                               |  (0x1u << (GPIO_PIN_2  * 2))
                               |  (0x1u << (GPIO_PIN_3  * 2))
                               |  (0x1u << (GPIO_PIN_4  * 2))
                               |  (0x1u << (GPIO_PIN_5  * 2))
                               |  (0x1u << (GPIO_PIN_11 * 2))
                               |  (0x1u << (GPIO_PIN_12 * 2))
                               |  (0x1u << (GPIO_PIN_13 * 2))
                               |  (0x1u << (GPIO_PIN_14 * 2))
                               |  (0x1u << (GPIO_PIN_15 * 2))
                               ;
  //
  // GPIOG Init
  // Pin 0,1,2,4,5,8,15
  // Alternate function 12
  //
  GPIOx_AFSEL0(GPIOG_BASEADDR) |= (0xCu << (GPIO_PIN_0 * 4))
                               |  (0xCu << (GPIO_PIN_1 * 4))
                               |  (0xCu << (GPIO_PIN_2 * 4))
                               |  (0xCu << (GPIO_PIN_4 * 4))
                               |  (0xCu << (GPIO_PIN_5 * 4))
                               ;
  GPIOx_AFSEL1(GPIOG_BASEADDR) |= (0xCu << ((GPIO_PIN_8  - GPIO_PIN_8) * 4))
                               |  (0xCu << ((GPIO_PIN_15 - GPIO_PIN_8) * 4))
                               ;
  //
  // Set Alternate Function Push Pull Mode (0x02)
  //
  GPIOx_CTL(GPIOG_BASEADDR)    |= (0x2u << (GPIO_PIN_0  * 2))
                               |  (0x2u << (GPIO_PIN_1  * 2))
                               |  (0x2u << (GPIO_PIN_2  * 2))
                               |  (0x2u << (GPIO_PIN_4  * 2))
                               |  (0x2u << (GPIO_PIN_5  * 2))
                               |  (0x2u << (GPIO_PIN_8  * 2))
                               |  (0x2u << (GPIO_PIN_15 * 2))
                               ;
  //
  // set GPIO_SPEED to "fast speed" = 0x02
  //
  GPIOx_OSPD(GPIOG_BASEADDR)   |= (0x2u << (GPIO_PIN_0  * 2))
                               |  (0x2u << (GPIO_PIN_1  * 2))
                               |  (0x2u << (GPIO_PIN_2  * 2))
                               |  (0x2u << (GPIO_PIN_4  * 2))
                               |  (0x2u << (GPIO_PIN_5  * 2))
                               |  (0x2u << (GPIO_PIN_8  * 2))
                               |  (0x2u << (GPIO_PIN_15 * 2))
                               ;
  //
  // set GPIO_PULLUP("Pull-up activation") (0x01)
  //
  GPIOx_PUD(GPIOG_BASEADDR)    |= (0x1u << (GPIO_PIN_0  * 2))
                               |  (0x1u << (GPIO_PIN_1  * 2))
                               |  (0x1u << (GPIO_PIN_2  * 2))
                               |  (0x1u << (GPIO_PIN_4  * 2))
                               |  (0x1u << (GPIO_PIN_5  * 2))
                               |  (0x1u << (GPIO_PIN_8  * 2))
                               |  (0x1u << (GPIO_PIN_15 * 2))
                               ;
  //
  // GPIOH Init
  // Pin 0,1,2,3,4,5,6,7,9,10
  // Alternate function 12
  //
  GPIOx_AFSEL0(GPIOH_BASEADDR) |= (0xCu << (GPIO_PIN_5 * 4));
  GPIOx_CTL(GPIOH_BASEADDR)    |= (0x2u << (GPIO_PIN_5 * 2));  // Set Alternate Function Push Pull Mode (0x02)
  GPIOx_OSPD(GPIOH_BASEADDR)   |= (0x2u << (GPIO_PIN_5 * 2));  // set GPIO_SPEED to "fast speed" = 0x02
  GPIOx_PUD(GPIOH_BASEADDR)    |= (0x1u << (GPIO_PIN_5 * 2));  // set GPIO_PULLUP("Pull-up activation") (0x01)
}

/*********************************************************************
*
*       _SDRAM_InitSequence()
*/
static void _SDRAM_InitSequence(void) {
  unsigned int Cmd;

  //
  // Configure a clock configuration enable command
  //
  Cmd = 0u
      | CMD_CLK_ENABLE                   // Mode
      | CMD_TARGET_BANK1                 // Target bank
      | ((CMD_AUTOREFRESH_1 - 1u) << 5)  // Auto refresh number
      | (0u << 9)                        // Mode register definition
      ;
  //
  // Send command
  //
  EXMC_SDCMD = Cmd;
  while (EXMC_SDSTAT & 0x20u) {
    ;
  }
  //
  // Configure a PALL (precharge all) command
  //
  Cmd = 0u
      | CMD_PALL                         // Mode
      | CMD_TARGET_BANK1                 // Target bank
      | ((CMD_AUTOREFRESH_1 - 1u) << 5)  // Auto refresh number
      | (0u << 9)                        // Mode register definition
      ;
  //
  // Send command
  //
  EXMC_SDCMD = Cmd;
  while (EXMC_SDSTAT & 0x20u) {
    ;
  }
  //
  // Configure an Auto Refresh command
  //
  Cmd = 0u
      | CMD_AUTOREFRESH_MODE             // Mode
      | CMD_TARGET_BANK1                 // Target bank
      | ((CMD_AUTOREFRESH_8 - 1u) << 5)  // Auto refresh number
      | (0u << 9)                        // Mode register definition
      ;
  //
  // Send command
  //
  EXMC_SDCMD = Cmd;
  while (EXMC_SDSTAT & 0x20u) {
    ;
  }
  //
  // Program the external memory mode register
  //
  Cmd = 0u
      | CMD_LOAD_MODE                    // Mode
      | CMD_TARGET_BANK1                 // Target bank
      | ((CMD_AUTOREFRESH_1 - 1u) << 5)  // Auto refresh number
      | (0x0230u << 9)                   // Mode register definition:
      ;                                  //   Burst length:     1
                                         //   Burst type:       Sequential
                                         //   CAS latency:      3
                                         //   Operating mode:   Standard
                                         //   Write burst mode: Single
  //
  // Send command
  //
  EXMC_SDCMD = Cmd;
  while (EXMC_SDSTAT & 0x20u) {
    ;
  }
  //
  // Set the refresh rate counter
  //
  Cmd        = EXMC_SDARI & (~0x3FFEu);
  EXMC_SDARI = Cmd | ((REFRESH_COUNT << 1) & 0x3FFEu);
}

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*       MemoryInit()
*
*  Function description
*    Initializes memories.
*
*  Notes
*    This routine is called before the segment initialization.
*
*    MemoryInit() is called from the Embedded Studio startup code
*    when the define MEMORY_INIT is set.
*    __low_level_init() is called from the IAR startup code.
*/
#if (defined(__ICCARM__))
INTERWORK int __low_level_init(void) {
#else
void MemoryInit(void) {
#endif
  //
  // Pin and DMA initialization
  //
  _PinInit();
  //
  // Enable EXMC clock
  //
  RCU_AHB3ENR |= 1u;
  //
  // EXMC Configuration
  // EXMC SDRAM Bank configuration
  //
  EXMC_SDCTL0  = 0x000039D9u;
  //
  // Timing configuration
  //
  EXMC_SDTCFG0 = 0x01115461u;
  //
  // SDRAM initialization sequence
  //
  _SDRAM_InitSequence();
#if (defined(__ICCARM__))
  return 1;       // Always return 1 to enable segment initilization!
#endif
}

/*************************** End of file ****************************/
