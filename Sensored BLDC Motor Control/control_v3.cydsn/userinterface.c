/*******************************************************************************
* Project Name		: Sensored BLDC Motor Control
* File Name			: userinterface.c
* Version			: 1.0
* Device Used		: CY8C4245AXI-483     
* Software Used		: PSoC Creator 4.2
* Compiler Used		: ARM GCC 5.4.1 
* Related Hardware  : CY8CKIT-042 PSoC 4 Pioneer Kit + CY8CKIT-037 PSoC 4
*                     Motor Control Evaluation Kit
******************************************************************************
* Copyright (2018), Cypress Semiconductor Corporation. All rights reserved.
*******************************************************************************
* This software, including source code, documentation and related materials
* (“Software”), is owned by Cypress Semiconductor Corporation or one of its
* subsidiaries (“Cypress”) and is protected by and subject to worldwide patent
* protection (United States and foreign), United States copyright laws and
* international treaty provisions. Therefore, you may use this Software only
* as provided in the license agreement accompanying the software package from
* which you obtained this Software (“EULA”).
*
* If no EULA applies, Cypress hereby grants you a personal, nonexclusive,
* non-transferable license to copy, modify, and compile the Software source
* code solely for use in connection with Cypress’s integrated circuit products.
* Any reproduction, modification, translation, compilation, or representation
* of this Software except as specified above is prohibited without the express
* written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, 
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED 
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress 
* reserves the right to make changes to the Software without notice. Cypress 
* does not assume any liability arising out of the application or use of the 
* Software or any product or circuit described in the Software. Cypress does 
* not authorize its products for use in any products where a malfunction or 
* failure of the Cypress product may reasonably be expected to result in 
* significant property damage, injury or death (“High Risk Product”). By 
* including Cypress’s product in a High Risk Product, the manufacturer of such 
* system or application assumes all risk of such use and in doing so agrees to 
* indemnify Cypress against all liability.
*******************************************************************************/
#include <project.h>
#include "motor.h"
#include "getvalue.h"
#include "userinterface.h"


/******************************************************************************
 * Global variables definition
 * ----------------------------------------------------------------------------
 * These variables should be populated to other modules. Header file contains 
 * the extern statement for these variables.
 ******************************************************************************/ 

/* press status for button SW2, default is BUTTON_OFF */
uint8 btnStatus[BTN_COUNT];

/******************************************************************************
 * Local Macro definition
 * ----------------------------------------------------------------------------
 * These Macros are only used in this module. These Macros should not be 
 * populated to other modules.
 ******************************************************************************/
#define BTN_LOW_LEVEL                       (uint8)(0x00)
#define BTN_HIHG_LEVEL                      (uint8)(0x01)

/* this macro defines the initial level when starting to detect button pressing */
#define BTN_OFF_LEVEL                       BTN_HIHG_LEVEL
/* this macro defines the desired level when button is pressed */
#define BTN_ON_LEVEL                        BTN_LOW_LEVEL

/* debounce count for button glitch filter */
#define BTN_GLITCH_FILTER_ACTIVE_CNT        (uint8)(15)
#define BTN_GLITCH_FILTER_DISCARD_CNT       (uint8)(5)
#define BTN_GLITCH_FILTER_INIT_CNT          (uint8)(10)

/******************************************************************************
 * Local variables definition
 * ----------------------------------------------------------------------------
 * These variables are only used in this module. These variables should not be 
 * populated to other modules.
 ******************************************************************************/
typedef struct _Btn_Status_T
{
    uint8   preBtnStatus;                   /* variable to store last status of button level  */
    uint8   glitchFilter;                   /* glitch filter counter for button pressing */
    uint8   btnIsDetectFlag;                 /* flag to store button pressing event */ 
}Btn_Status_T;

static Btn_Status_T btnArray[BTN_COUNT];

static uint8 bcpTxBuffer[32];

// Variables from example Tx code
static uint8 bcpRxBuffer[16];
static uint8 bcpRxBufferIx = 0;
uint8_t bcpRxFlag = 0;

// Communication Variables
uint32 tempOutputSpeedRef = 0;
uint16 tempAngularPos = 0;

/*******************************************************************************
* Function Name: Button_Init
********************************************************************************
* Summary:
*   Initialize button status structure to detect pressing event
*
* Parameters:  
*   void
*
* Return: 
*   void
*
*******************************************************************************/
void ButtonInit(void)
{
    uint8 i = 0;
    
    for(i = 0; i < BTN_COUNT; i++)
    {
        btnArray[i].preBtnStatus = BTN_OFF_LEVEL;
        btnArray[i].glitchFilter = BTN_GLITCH_FILTER_INIT_CNT;
        btnArray[i].btnIsDetectFlag = FALSE;
    }
}

/*******************************************************************************
* Function Name: ButtonPressDetect
********************************************************************************
* Summary:
*   Detect button status for pressing event
*
* Parameters:  
*   void
*
* Return: 
*   void
*
*******************************************************************************/
void ButtonPressDetect(uint8 btnStatus, Btn_Status_T* btnArrayPtr, uint8* btnStatusPtr)
{
    uint8 curBtnStatus = btnStatus;    /* get current button level status */
    if(btnArrayPtr->btnIsDetectFlag == FALSE)          
    {
        /* detect button pressing event */
        btnArrayPtr->btnIsDetectFlag = ((curBtnStatus == BTN_ON_LEVEL) && 
                                       (curBtnStatus ^ btnArrayPtr->preBtnStatus)) ? TRUE : FALSE;
    }
    else
    {
        if(curBtnStatus == BTN_ON_LEVEL)   /* button keeps in desired level status */
        {            
            /*  detect if glitch filter counter value is larger than pre-defined threshold */
            if(btnArrayPtr->glitchFilter > BTN_GLITCH_FILTER_ACTIVE_CNT)
            {
                /* succeed in button pressing detection, invert button ON/OFF status */
                *btnStatusPtr = *btnStatusPtr ^ BUTTON_ON;
                /* reset glitch filter counter */
                btnArrayPtr->glitchFilter = BTN_GLITCH_FILTER_INIT_CNT;
                /* clear detection flag */
                btnArrayPtr->btnIsDetectFlag = FALSE;
            }
            else
            {
                /* increase glitch filter counter */
                btnArrayPtr->glitchFilter++; 
            }
        }
        else
        {
            if(btnArrayPtr->glitchFilter < BTN_GLITCH_FILTER_DISCARD_CNT)
            {
                /* keep buttons status unchanged, reset glitch filter counter */
                btnArrayPtr->glitchFilter = BTN_GLITCH_FILTER_INIT_CNT;
                /* clear detection flag */
                btnArrayPtr->btnIsDetectFlag = FALSE;
            }
            else
            {
                 /* decrease glitch filter counter */
                btnArrayPtr->glitchFilter--; 
            }
        }
    }
    /* update last button status with current status */
    btnArrayPtr->preBtnStatus = curBtnStatus;
}

/*******************************************************************************
* Function Name: ButtonProcess
********************************************************************************
* Summary:   
* This function detects the press action of button.
*
* Parameters: None 
*  
* Return: None
*
*******************************************************************************/
void ButtonProcess(void)
{
    ButtonPressDetect(SW2_Read(), &btnArray[0], &btnStatus[0]);
    if(btnStatus[0] == BUTTON_ON)
    {
        UI_Cmd.run = TRUE;	
	    stateSys = STATUS_RUN;
    }
    else
    {
        UI_Cmd.run = FALSE; 	
	    stateSys = STATUS_STOP;
    }
}

/*******************************************************************************
* Function Name: UpdateStatusStart
********************************************************************************
* Summary:   
* This function initialize the motor, and ready to start.
*
* Parameters: None 
*  
* Return: None
*
*******************************************************************************/
void UpdateStatusInit(void)
{
	UI_Cmd.run = FALSE;
	stateSys = STATUS_STOP;	
}

/*******************************************************************************
* Function Name: UpdateStatusError
********************************************************************************
* Summary:   
* This function stop the motor and update error flag.
*
* Parameters: None 
*  
* Return: None
*
*******************************************************************************/
void UpdateStatusError()
{	
	stateSys = STATUS_ERROR;	
}

/****************************************************************************************
*                                                                             
*   Function:    BCPPoll                                                        
*                                                                             
*   Description: BCPPoll routine                                                   
*             Format:  RX8 [h=55] @0speed @1speed [t=AA]                      
*   Parameters:   None                                                          
*                                                                                 
*   Returns:     None                                                                 
*                                                                                   
****************************************************************************************/

void BCPPoll()
{
    uint8 index = 0;
    
    if(UART_BCP_SpiUartGetTxBufferSize())
       return;
    
    /* package header */
    bcpTxBuffer[index++] = 0x55;
    
    //if a message has been received, echo that message    
    if (bcpRxFlag > 0 ) {
        for (index = 0; index < bcpRxBufferIx; index++)
        {
            bcpTxBuffer[index] = bcpRxBuffer[index];
        }    
        UART_BCP_SpiUartPutArray(bcpTxBuffer, index);
        bcpRxBufferIx=0;
        
        bcpRxFlag = 0;
    }
    // else send back meaured data
    else {
    
        /* construct BCP data package with speed value, MSB first */    
        
        /* UI_Data.test */
        bcpTxBuffer[index++] = (uint8)((UI_Data.test & 0xFF00) >> 8);
        bcpTxBuffer[index++] = (uint8)(UI_Data.test & 0x00FF); 
        
        /* UI_Data.test2 */
        bcpTxBuffer[index++] = (uint8)((UI_Data.test2 & 0xFF00) >> 8);
        bcpTxBuffer[index++] = (uint8)(UI_Data.test2 & 0x00FF); 
        
        /* UI_Data.programStateManchineState */
        bcpTxBuffer[index++] = (uint8)((UI_Data.programStateManchineState & 0xFF00) >> 8);
        bcpTxBuffer[index++] = (uint8)(UI_Data.programStateManchineState & 0x00FF); 
        
        /* current measured speed */
        bcpTxBuffer[index++] = (uint8)((UI_Data.speedRpm & 0xFF00) >> 8);
        bcpTxBuffer[index++] = (uint8)(UI_Data.speedRpm & 0x00FF); 
        
        /* gearbox output speed reference */
//        bcpTxBuffer[index++] = (uint8)((UI_Data.speedRpmRef & 0xFF00) >> 8);
//        bcpTxBuffer[index++] = (uint8)(UI_Data.speedRpmRef & 0x00FF); 
        
        /* motor output speed target */
        tempOutputSpeedRef = (uint16)UI_Data.speedRpmRef * (uint16)UI_Data.gearRatio;
        bcpTxBuffer[index++] = ((tempOutputSpeedRef & 0x0000FF00) >> 8);
        bcpTxBuffer[index++] = (tempOutputSpeedRef & 0x000000FF); 
        
        /* motor direction */
        bcpTxBuffer[index++] = ((UI_Data.Dir & 0xFF00) >> 8);
        bcpTxBuffer[index++] = (UI_Data.Dir & 0x00FF);     
        
        /* gearbox output angular position */
        tempAngularPos = (uint16)UI_Data.angularPos;
        bcpTxBuffer[index++] = ((tempAngularPos & 0xFF00) >> 8);
        bcpTxBuffer[index++] = (tempAngularPos & 0x00FF);      
        
        /* gearbox output complete rotation count */
//        bcpTxBuffer[index++] = ((UI_Data.spinCount & 0xFF00) >> 8);
//        bcpTxBuffer[index++] = (UI_Data.spinCount & 0x00FF);    

        /* motor winding current (currently not functional) */
//        bcpTxBuffer[index++] = ((Pin_Ibus_ReadDataReg() & 0xFF00) >> 8);
//        bcpTxBuffer[index++] = (Pin_Ibus_ReadDataReg() & 0x00FF);
        
        /* UI_Data.timeBetweenHallSamples */
//        bcpTxBuffer[index++] = (uint8)((UI_Data.timeBetweenHallSamples & 0xFF00) >> 8);
//        bcpTxBuffer[index++] = (uint8)(UI_Data.timeBetweenHallSamples & 0x00FF); 
        
        /* test to ensure data is being dynamically read by serial communication on external connected device (note that the test variable on top of this file must be uncommented) */
//        bcpTxBuffer[index++] = (uint8)((test & 0xFF00) >> 8);
//        bcpTxBuffer[index++] = (uint8)((test & 0x00FF));
//        bcpTxBuffer[index++] = (uint8)((test & 0xFF00) >> 8);
//        bcpTxBuffer[index++] = (uint8)((test & 0x00FF));
//        
//        test++;
        
        /* package tail */
        bcpTxBuffer[index++] = 0xAA;
        
        UART_BCP_SpiUartPutArray(bcpTxBuffer, index);
    }
}

/****************************************************************************************
*																			  			*
*   Function:    BCPRxPoll								  								*
*																			  			*
*   Description: Polls receive buffer. Copies and processes data   						*
*				 Format:  RX8 [h=55] 'M' mode 'm' submode [t=AA]                        *
*                Where mode = 0 (standby), 1 BLAC, 2 DC mmotor, 3 Inductor              *
*                submode =1 (open loop AC), 2 (open loop DC), 3 (closed loop DC)        *
*                only used for inductor                                                 *
*                                                                                       *
*                This is from example Rx Tx code                                        *
*                                                                                       *
*   Parameters:  None																	*
*				  							  			                                *
*   Returns:     None - but changes global variables									*		
*																			  			*		
****************************************************************************************/

void BCPRxPoll()
{
uint8 temp;

    if (UART_BCP_SpiUartGetRxBufferSize() > 16) // buffer overflow
    {
//        MotorCtrl_stcRunPar.u32ErroType |= COMMS_ERROR; // buffer overflow
        UART_BCP_SpiUartClearRxBuffer();
        return;
    }
    if (bcpRxFlag == 1)  //not processed previous command
    { 
        return;
    }
    while (UART_BCP_SpiUartGetRxBufferSize() > 0 )
    {
        temp = (uint8)UART_BCP_SpiUartReadRxData();
        if ((temp == 0x55) | (bcpRxBufferIx > 0))
        {
            bcpRxBuffer[bcpRxBufferIx]=temp;
            bcpRxBufferIx++;
        }
        if ((temp == 0xaa) |(bcpRxBufferIx > 6))
        {
            bcpRxBuffer[bcpRxBufferIx]=temp;
            bcpRxFlag=1;
            UART_BCP_SpiUartClearRxBuffer();
            break;
        }
    }
    
}
/* [] END OF FILE */

