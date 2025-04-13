/*******************************************************************************
* File Name: clock_3.h
* Version 2.20
*
*  Description:
*   Provides the function and constant definitions for the clock component.
*
*  Note:
*
********************************************************************************
* Copyright 2008-2012, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#if !defined(CY_CLOCK_clock_3_H)
#define CY_CLOCK_clock_3_H

#include <cytypes.h>
#include <cyfitter.h>


/***************************************
*        Function Prototypes
***************************************/
#if defined CYREG_PERI_DIV_CMD

void clock_3_StartEx(uint32 alignClkDiv);
#define clock_3_Start() \
    clock_3_StartEx(clock_3__PA_DIV_ID)

#else

void clock_3_Start(void);

#endif/* CYREG_PERI_DIV_CMD */

void clock_3_Stop(void);

void clock_3_SetFractionalDividerRegister(uint16 clkDivider, uint8 clkFractional);

uint16 clock_3_GetDividerRegister(void);
uint8  clock_3_GetFractionalDividerRegister(void);

#define clock_3_Enable()                         clock_3_Start()
#define clock_3_Disable()                        clock_3_Stop()
#define clock_3_SetDividerRegister(clkDivider, reset)  \
    clock_3_SetFractionalDividerRegister((clkDivider), 0u)
#define clock_3_SetDivider(clkDivider)           clock_3_SetDividerRegister((clkDivider), 1u)
#define clock_3_SetDividerValue(clkDivider)      clock_3_SetDividerRegister((clkDivider) - 1u, 1u)


/***************************************
*             Registers
***************************************/
#if defined CYREG_PERI_DIV_CMD

#define clock_3_DIV_ID     clock_3__DIV_ID

#define clock_3_CMD_REG    (*(reg32 *)CYREG_PERI_DIV_CMD)
#define clock_3_CTRL_REG   (*(reg32 *)clock_3__CTRL_REGISTER)
#define clock_3_DIV_REG    (*(reg32 *)clock_3__DIV_REGISTER)

#define clock_3_CMD_DIV_SHIFT          (0u)
#define clock_3_CMD_PA_DIV_SHIFT       (8u)
#define clock_3_CMD_DISABLE_SHIFT      (30u)
#define clock_3_CMD_ENABLE_SHIFT       (31u)

#define clock_3_CMD_DISABLE_MASK       ((uint32)((uint32)1u << clock_3_CMD_DISABLE_SHIFT))
#define clock_3_CMD_ENABLE_MASK        ((uint32)((uint32)1u << clock_3_CMD_ENABLE_SHIFT))

#define clock_3_DIV_FRAC_MASK  (0x000000F8u)
#define clock_3_DIV_FRAC_SHIFT (3u)
#define clock_3_DIV_INT_MASK   (0xFFFFFF00u)
#define clock_3_DIV_INT_SHIFT  (8u)

#else 

#define clock_3_DIV_REG        (*(reg32 *)clock_3__REGISTER)
#define clock_3_ENABLE_REG     clock_3_DIV_REG
#define clock_3_DIV_FRAC_MASK  clock_3__FRAC_MASK
#define clock_3_DIV_FRAC_SHIFT (16u)
#define clock_3_DIV_INT_MASK   clock_3__DIVIDER_MASK
#define clock_3_DIV_INT_SHIFT  (0u)

#endif/* CYREG_PERI_DIV_CMD */

#endif /* !defined(CY_CLOCK_clock_3_H) */

/* [] END OF FILE */
