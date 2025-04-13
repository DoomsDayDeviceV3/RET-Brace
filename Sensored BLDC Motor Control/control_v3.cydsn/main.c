/*******************************************************************************
* Project Name		: Sensored BLDC Motor Control
* File Name			: main.c
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
#include "speed.h"
#include "userinterface.h"
#include "getvalue.h"

#define myFloor(x) ((int)(x) - ((x) < 0 && (x) != (int)(x)));
#define myAbs(x) ((x) < 0.0 ? -(x) : (x));

/* 1 = brace control, 0 = simple speed control */
#define NORMALCONTROL 1
#define SPEEDCONTROL (!NORMALCONTROL)

int main()
{  
    uint8 HallReader = 0;    
	uint16 pwmCntLocal = 0;
    
    /* initialize parameters in UI FW */
    Init_UI_FW();    
    /* disable global interrupt */ 
	CyGlobalIntDisable;	
    /* initialize system hardware components */	
    Init_HW();   	
    /*Enable Global interrupt*/
	CyGlobalIntEnable;    
    /* initialize UI hardware components */ 
	Init_UI_HW();
    
    /* stop motor as default */
	UI_Cmd.run =  FALSE;     
    /* Set Motor Direction */
    /* CLOCK_WISE = 0x01, and COUNTER_CLOCK_WISE = 0x00 */ 
    
    #if SPEEDCONTROL
	UI_Data.Dir = CLOCK_WISE;	
    #endif
        
    CtrlReg_Dir_Write(UI_Data.Dir);
    
    for(;;)
    {
        /* Scan sensors and handle event */
	    ButtonProcess();
		
        /* Measure bus voltage */
        VoltageCheck();	
        
        #if NORMALCONTROL
        /* Change direction of brace movement at +45 and -45 degrees of arc */
        if (
            (UI_Data.programStateManchineState > 1) &&
            (
            ((UI_Data.Dir == CLOCK_WISE) && (UI_Data.angularPos >= 45.0)) || 
            ((UI_Data.Dir == COUNTER_CLOCK_WISE) && (UI_Data.angularPos <= -45.0))
            )
        ) {
            UI_Data.changeDirection = TRUE;
            UI_Cmd.run = FALSE;
            
            UI_Data.programStateManchineState++;
        }
        
        /* wait period before begining of exercise arc - COMPLETE THIS CODE */
        if (UI_Data.programStateManchineState == 1) { // make this a condition depending on a timer
            UI_Data.programStateManchineState = 2;
        }
        
        /* wait period after direction change of excercise arc - COMPLETE THIS CODE */
        if (UI_Data.programStateManchineState == 5) { // make this a condition depending on a timer
            UI_Data.programStateManchineState = 6;
        }
            
        /* Change acceleration of brace response at start and end of arc by incrementing state machine */
        if ( 
            ((UI_Data.programStateManchineState == 2) && (UI_Data.angularPos >= -10.0)) ||
            ((UI_Data.programStateManchineState == 3) && (UI_Data.angularPos >= 10.0)) ||
            ((UI_Data.programStateManchineState == 6) && (UI_Data.angularPos <= 10.0)) ||
            ((UI_Data.programStateManchineState == 7) && (UI_Data.angularPos <= -10.0))
        ) {
            UI_Data.programStateManchineState++;
        }
        
        /* End of exercise - stop program */
        if (UI_Data.programStateManchineState == 9) {
            UI_Cmd.run = FALSE;
        }
        #endif
        
        /* Erroneus state machine state*/
        if (UI_Data.programStateManchineState > 9) {
            errorState = stateMachineError;
        }

        if ((UI_Cmd.run == FALSE) && (UI_Data.changeDirection == TRUE)) {
            CtrlReg_PWMOut_Write(0x00);  
            
            #if NORMALCONTROL
            if (UI_Data.Dir == CLOCK_WISE) {
                UI_Data.Dir = COUNTER_CLOCK_WISE;
            } else if (UI_Data.Dir == COUNTER_CLOCK_WISE) {
                UI_Data.Dir = CLOCK_WISE;
            } else {
                errorState = dirError;
            }
            #endif

        	pwmCntLocal = 0;
            pwmCnt = 0;
             
            CtrlReg_Dir_Write(UI_Data.Dir);
            
            UI_Data.changeDirection = FALSE;
            
            CyDelay(2000);
              
            drive_reset();
            
        	pwmCntLocal = 0;
            pwmCnt = 0;
            
            UI_Cmd.run = TRUE;
                
        } else if(UI_Cmd.run == FALSE) {
            /*Stop motor by disabling PWM output */
            /* Turning off LED when motor stopping */
			STATUS_LED_Write(1);			
        	CtrlReg_PWMOut_Write(0x00);  
			PWM_Drive_WriteCompare(PWM_Drive_ReadPeriod()>>16);
            
        	piOut = INIT_PIOUT;
        	HallReader = TRUE;
        } 
		
		/* Error Protection: Disable PWM and shining LED at 1Hz */
		if(errorState != 0)
		{
			CtrlReg_PWMOut_Write(0x00);
            
            /* Shining LED at 0.1Hz */
            // errorState values: no_error = 0, overCur, hallError, lowVolt, highVolt, dirError, stateMachineError
            if(errorState == overCur) {
                STATUS_LED_Write(~(STATUS_LED_Read()));
                CyDelay(5000);
                STATUS_LED_Write(~(STATUS_LED_Read()));
                CyDelay(5000);
            }

            /* Shining LED at 1Hz */
            else{
                STATUS_LED_Write(~(STATUS_LED_Read()));
                CyDelay(500);
                STATUS_LED_Write(~(STATUS_LED_Read()));
                CyDelay(500);
            }
		}		
        
		/* Motor Running */
        if(UI_Cmd.run == TRUE && errorState == 0)
        {
			/* Lighting LED when motor running */
			STATUS_LED_Write(0);
			
            /* only send out UART data when motor is running */
		    BCPPoll();
            
            CtrlReg_PWMOut_Write(0x03);  
            
            pwmCntLocal = pwmCnt;
            

            /* Update every 12.5mS*/
           	if((pwmCntLocal & 0xff) == 0xff) /*if the number of PWM periods is 256, this if code runs*/
    	    {
                pwmCntLocal++;

                uint32 temp = 0;
                float64 temp2 = myAbs(UI_Data.angularPos);
                
                #if NORMALCONTROL
                if ((UI_Data.programStateManchineState == 2) || (UI_Data.programStateManchineState == 6)) {
                    temp = (uint32) ((float)UI_Data.speedRpmRef * ((float)UI_Data.gearRatio) * (1.0 - (temp2 - UI_Data.switchAngle)/(45.0 - UI_Data.switchAngle) ));
                }
                else if ((UI_Data.programStateManchineState == 4) || (UI_Data.programStateManchineState == 8)) {
                    temp = (uint32) ((float)UI_Data.speedRpmRef * ((float)UI_Data.gearRatio) * (1.0 - (temp2 - UI_Data.switchAngle)/(45.0 - UI_Data.switchAngle) ));
                } else {
                    temp = (uint32) (UI_Data.speedRpmRef * UI_Data.gearRatio);
                }
                
//                if (temp <= 10) {temp = 10;}
//                if (temp >= 500) {temp = 500;}
                #else
//                if (temp2 >= 200.0){temp = 400;} else {temp = 300;};
                temp = 200;
                #endif
                
                
                speedRef = (60*FREQ_CAPTURE_CLK)/(uint32)(MOTOR_POLE_PAIRS*temp);
    			UI_Data.test = (uint16) temp;
                
                if(UI_Cmd.run == TRUE)
                {
                /*Speed close loop control */            
                	SpeedPID();  
                }	 
    			
                /* Hall Error Detection*/
                HallReader  = Hall_Error_Read();
                if(HallReader == TRUE)
                {               
    				errorState = hallError;
                	UpdateStatusError();
                }			
    		}
            
            if(pwmCntLocal >= 500) {
                UI_Data.motorWindupOvercurrentDisable = 0; //enable overcurrent protection
            }
            
            pwmCnt = pwmCntLocal;
        }	
        
        /* Motor Not Running */
        else {
            BCPRxPoll();
        }
    }
}

/* [] END OF FILE */
