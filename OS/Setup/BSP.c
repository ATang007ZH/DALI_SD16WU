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
Purpose : BSP for GigaDevice GD32450I-Eval
*/

#include "BSP.h"

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/
#define NUM_LEDS    (3)

#define RCU_BASE_ADDR    (0x40023800u)                                        // Reset and clock unit base address
#define RCU_AHB1EN       ((volatile unsigned int*)(RCU_BASE_ADDR + 0x30u))    // AHB1 enable register

#define GPIOE_BASE_ADDR  (0x40021000u)                                        // GPIOE base address
#define GPIOE_CTL0       ((volatile unsigned int*)(GPIOE_BASE_ADDR + 0x00u))  // Port Control register 0
#define GPIOE_BOP        ((volatile unsigned int*)(GPIOE_BASE_ADDR + 0x18u))  // Port bit operate register
#define GPIOE_BC         ((volatile unsigned int*)(GPIOE_BASE_ADDR + 0x28u))  // Port bit clear register
#define GPIOE_TG         ((volatile unsigned int*)(GPIOE_BASE_ADDR + 0x2Cu))  // Port bit toggle register
#define GPIOE_OCTL       ((volatile unsigned int*)(GPIOE_BASE_ADDR + 0x14u))  // Port output control register

#define GPIOF_BASE_ADDR  (0x40021400u)                                        // GPIOF base address
#define GPIOF_CTL0       ((volatile unsigned int*)(GPIOF_BASE_ADDR + 0x00u))  // Port Control register 0
#define GPIOF_BOP        ((volatile unsigned int*)(GPIOF_BASE_ADDR + 0x18u))  // Port bit operate register
#define GPIOF_BC         ((volatile unsigned int*)(GPIOF_BASE_ADDR + 0x28u))  // Port bit clear register
#define GPIOF_TG         ((volatile unsigned int*)(GPIOF_BASE_ADDR + 0x2Cu))  // Port bit toggle register
#define GPIOF_OCTL       ((volatile unsigned int*)(GPIOF_BASE_ADDR + 0x14u))  // Port output control register

/*********************************************************************
*
*       Typedefs
*
**********************************************************************
*/
typedef struct _LED_INFO {
  int                    PortPin;
  volatile unsigned int* pCtlReg;
  volatile unsigned int* pBopReg;
  volatile unsigned int* pBcReg;
  volatile unsigned int* pTgReg;
  volatile unsigned int* pOctlReg;
} LED_INFO;

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static LED_INFO _aLEDInfo[] = {
  { 2, GPIOE_CTL0, GPIOE_BOP, GPIOE_BC, GPIOE_TG, GPIOE_OCTL },
  { 3, GPIOE_CTL0, GPIOE_BOP, GPIOE_BC, GPIOE_TG, GPIOE_OCTL },
  {10, GPIOF_CTL0, GPIOF_BOP, GPIOF_BC, GPIOF_TG, GPIOF_OCTL }
};

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*       BSP_Init()
*/
void BSP_Init(void) {
  int i;

  *RCU_AHB1EN |= 0x30u;                                             // Enable Port C and Port E clock
  for (i = 0; i < NUM_LEDS; i++) {
    *(_aLEDInfo[i].pCtlReg) |= (1u << (_aLEDInfo[i].PortPin * 2));  // Set Pin to output
    *(_aLEDInfo[i].pBcReg)   = (1u <<  _aLEDInfo[i].PortPin);       // Clear Pin bit
  }
}

/*********************************************************************
*
*       BSP_SetLED()
*/
void BSP_SetLED(int Index) {
  if (Index < NUM_LEDS) {
    *(_aLEDInfo[Index].pBopReg) = (1u << _aLEDInfo[Index].PortPin);
  }
}

/*********************************************************************
*
*       BSP_ClrLED()
*/
void BSP_ClrLED(int Index) {
  if (Index < NUM_LEDS) {
    *(_aLEDInfo[Index].pBcReg) = (1u << _aLEDInfo[Index].PortPin);
  }
}

/*********************************************************************
*
*       BSP_ToggleLED()
*/
void BSP_ToggleLED(int Index) {
  if (Index < NUM_LEDS) {
    *(_aLEDInfo[Index].pTgReg) = (1u << _aLEDInfo[Index].PortPin);
  }
}

/*************************** End of file ****************************/
