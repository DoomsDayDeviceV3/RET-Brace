/*******************************************************************************
* Project Name		: Sensored BLDC Motor Control
* File Name			: motor.c
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
#include "userinterface.h"
#include "getvalue.h"

/* UI_MOTOR_STATUS Status */
UI_CMD  UI_Cmd;
UI_DATA UI_Data;

uint16 gearRatio = 0;         /* :1 gearRatio */
uint16 speedRef = 0;            /* Motor speed Reference */
uint16 speedCur = 0;         /* Current motor speed. Original: 3000 */

uint16 preSpeedCur = 0;
uint16 preCntCaptur = 0;         /* Current motor speed. Original: 30000 */

float64 angularPos = 0;
int16 spinCount = 0;
uint8 changeDirection = FALSE;
uint8 programStateManchineState = 0;
float32 switchAngle = 10.0;

uint8  dutyCycle = 0;            /* really now used as BYTE since use 8-bit PWM */
uint16 pwmCnt = 0;
uint8 firstRun = 1;
uint8 ocBlankCnt = 0;
Error_T errorState = no_error;
uint8 stateSys = STATUS_STOP;

/*******************************************************************************
* Function Name: pwm_isr
********************************************************************************
*
* Summary:
* This function is PWM ISR. When TC happens, it increases PWM ticker, clears flag and 
* enables over current protection ISR after startup.
*
* Parameters: None
*
* Return: None
*
*******************************************************************************/
CY_ISR(pwm_isr)
{	
	pwmCnt++;  
	
    /* Avoid the current pulse interference in motor start-up*/
    if (UI_Cmd.run)
    {
        if(firstRun == 1)
        {
            ocBlankCnt++;
        }
        if (ocBlankCnt > 2)    /*Ignore the first 2 PWM period = 50uS*3=150uS, then enable over current ISR */
        {
            firstRun = 0;
            ocBlankCnt = 0;
            isr_oc_Enable();
            isr_oc_ClearPending();
        }
    }
	
	/* Calculate the real time motor speed every 2000 PWM period*/
	if(pwmCnt >= 2001)
	{
		pwmCnt = 0;
	}

	PWM_Drive_ClearInterrupt(PWM_Drive_INTR_MASK_TC);
}

/*******************************************************************************
* Function Name: drive_reset
********************************************************************************
*
* Summary:
* custom to allow direction change
* 
* Parameters: None
*
* Return: None
*
*******************************************************************************/
void drive_reset() {
    
    speedCur = 0;         /* Current motor speed. Original: 3000 */
    preSpeedCur = 0;
    preCntCaptur = 0; /* Original:30000*/
    UI_Data.timeBetweenHallSamples = 0;
    
    isr_oc_Stop();
    UI_Data.motorWindupOvercurrentDisable = 1;
    firstRun = 1;
    ocBlankCnt = 0;
    dutyCycle = 0;
    PWM_Drive_WriteCompare(dutyCycle);
}

/*******************************************************************************
* Function Name: time_isr
********************************************************************************
*
* Summary:
* custom to allow accurate speed sensing
* 
* Parameters: None
*
* Return: None
*
*******************************************************************************/
CY_ISR(time_isr) {
    UI_Data.timeBetweenHallSamples = UI_Data.timeBetweenHallSamples + 1;
    
    Timer_1_STATUS;
}

/*******************************************************************************
* Function Name: speed_measure_isr
********************************************************************************
*
* Summary:
* This function is ISR for motor speed and position measurement. 
* 
* Parameters: None
*
* Return: None
*
*******************************************************************************/
CY_ISR(speed_measure_isr)
{
    #if 0
    uint16 cntCaptur = 0;
    uint16 timeConst = 5405; // 4730 for 20-5 RPM, 5405 for 2 RPM 
    
    cntCaptur = Counter_Spd_ReadCapture();
	
	speedCur = preCntCaptur - cntCaptur;
	
    /* filter for speed measured */
//    speedCur = (uint16) ((float32)(3 * preSpeedCur + speedCur) / 4);
    speedCur = (preSpeedCur >> 2) + (preSpeedCur >> 1) + (speedCur >> 2);
    
    /* calculate hall effect sensor ticks for position sensing */
//    if (UI_Data.Dir == CLOCK_WISE) {
//        hallCount += speedCur;
//    } else if (UI_Data.Dir == COUNTER_CLOCK_WISE) {
//        hallCount -= speedCur;
//    }

    /* Each electrical revolution = 6 Hall transitions (for 3 Hall sensors in a BLDC motor) */
	/* Each mechanical revolution = 4 electrical revolutions (since there are 4 pole pairs) */
    /* Each motor degree of rotation/position = 1/360 mechanical revolutions */
    /* Each gearbox output rotation/position = gear ratio motor degree of rotation/position */
    /*  360.0 / (6.0 * 4.0) = 15*/
    if (UI_Data.Dir == CLOCK_WISE) {
        UI_Data.angularPos += ((float) cntCaptur) * 360.0 / (6.0 * 4.0) / ((float) UI_Data.gearRatio) / ((float) timeConst);
    } else if (UI_Data.Dir == COUNTER_CLOCK_WISE) {
        UI_Data.angularPos -= ((float) cntCaptur) * 360.0 / (6.0 * 4.0) / ((float) UI_Data.gearRatio) / ((float) timeConst);
    }
//    float32 temp3 = ((float) UI_Data.timeBetweenHallSamples) * 0.001f;
//    if (UI_Data.timeBetweenHallSamples <= 100) {
//        if (UI_Data.Dir == CLOCK_WISE) {
//            UI_Data.angularPos += ((float)speedCur) / 60.0 * temp3 * 15.0 / ((float) UI_Data.gearRatio);
//        } else if (UI_Data.Dir == COUNTER_CLOCK_WISE) {
//            UI_Data.angularPos -= ((float)speedCur) /60.0 * temp3 * 15.0 / ((float) UI_Data.gearRatio);
//        }
//    }
    
    
    if (UI_Data.angularPos >= 360.0) { // clockwise
        while (UI_Data.angularPos >= 360.0) {
            UI_Data.angularPos -= 360.0;
            UI_Data.spinCount++;
        }
    } else if (UI_Data.angularPos <= -360.0) { //anticlockwise
        while (UI_Data.angularPos <= -360.0) {
            UI_Data.angularPos += 360.0;
            UI_Data.spinCount--;
        }
    }

    preCntCaptur = cntCaptur;
	preSpeedCur = speedCur; 
    UI_Data.timeBetweenHallSamples = 0;
	
	Counter_Spd_ClearInterrupt(Counter_Spd_INTR_MASK_CC_MATCH);
    
    #else
    
    uint16 cntCaptur = Counter_Spd_ReadCapture();
    static uint16 preCntCaptur = 0;
    static float preSpeedCur = 0;
    
    // Compute the time difference (handle overflow)
    speedCur = preCntCaptur - cntCaptur;
//    if (speedCur < 0) {
//        speedCur += 65535; // Timer overflow handling
//    }

    // Apply filtering (EMA)
//    speedCur = (0.25f * preSpeedCur) + (0.75f * speedCur);
    
    // Convert to RPM (assuming 200 kHz clock) , 4 Hall pulses per rev as only one hall sensor is used, and there are 4 pole pairs)
    // Assuming 100 kHz clock
    // Assuming 4 Hall pulses per Revolution (only one hall sensor used, 4 Pole Pairs)
    float speedRpmFloat = (60.0f * 100000.0f) / ((float)speedCur * 4.0f);
    UI_Data.speedRpm = speedRpmFloat;
    
    // Compute delta angle (degrees)
    float dt = UI_Data.timeBetweenHallSamples * 0.001f; // Convert ms to seconds
    float angularSpeed = speedRpmFloat * 6.0f; // Convert RPM to degrees per second
    float deltaAngle = angularSpeed * dt;

    // Update angular position
    if (UI_Data.timeBetweenHallSamples <= 100) {
        if (UI_Data.Dir == CLOCK_WISE) {
            UI_Data.angularPos += deltaAngle / UI_Data.gearRatio;
        } else {
            UI_Data.angularPos -= deltaAngle / UI_Data.gearRatio;
        }
    }

    // Prevent overflow of angular position
    if (UI_Data.angularPos >= 360.0f) {
        UI_Data.angularPos -= 360.0f;
        UI_Data.spinCount++;
    } else if (UI_Data.angularPos <= -360.0f) {
        UI_Data.angularPos += 360.0f;
        UI_Data.spinCount--;
    }

    // Store values for next cycle
    preCntCaptur = cntCaptur;
    preSpeedCur = speedCur;
    UI_Data.timeBetweenHallSamples = 0;
    
    // Clear interrupt flag
    Counter_Spd_ClearInterrupt(Counter_Spd_INTR_MASK_CC_MATCH);
    #endif
}

/*******************************************************************************
* Function Name: over_current_isr
********************************************************************************
*
* Summary:
* This function is ISR for over current protection. It updates the error state flag. 
*                  
* Parameters: None
*
* Return: None
*
*******************************************************************************/
CY_ISR(over_current_isr)
{	
    if (UI_Data.motorWindupOvercurrentDisable == 0) {
        if (UI_Data.programStateManchineState == 0) {   /* Initial homing complete - continue to next state machine state */
            UI_Data.programStateManchineState = 2;
            
            UI_Data.changeDirection = TRUE;
            UI_Data.angularPos = -55.0;
            
            resetPIOut();
            
            UI_Cmd.run = FALSE;
            
//            IDAC_Iref_SetValue(MAX_CURR_DEFAULT);
        } else {                                        /* Unforseen overcurrent happens, stop motor*/
            UpdateStatusError();		
            errorState = overCur;
            LPComp_OC_ClearInterrupt(LPComp_OC_INTR_RISING);
        }
    }
}

/*******************************************************************************
* Function Name: Init_UI_FW
********************************************************************************
* Summary: This function initializes parameters used in motor running.
*  
* Parameters: None  
*
* Return: None
*  
*******************************************************************************/

void Init_UI_FW(void)
{
    /* Setting UI Initial parameter*/
	UI_Data.Dir = COUNTER_CLOCK_WISE;				
    UI_Data.maxSpeedRpm = 4000;
    UI_Data.minSpeedRpm = 0; /* used to be 500, reduced to accomodate low brace rpm*/
    UI_Data.speedRpmRef = 4;
    UI_Data.gearRatio = 100;
    UI_Data.polePairs = 4;
    UI_Data.maxCurr = MAX_CURR_MEDIUM; 
    UI_Data.angularPos = 0;
    UI_Data.kp = 10;   // original: 500
    UI_Data.ki = 10;    // original: 50
	
    UI_Data.motorWindupOvercurrentDisable = 1;  // turned off after first few PWM cycles
}


/*******************************************************************************
* Function Name: Init_HW
********************************************************************************
* Summary: This function initializes system hardware peripherals 
*  
* Parameters: None  
*
* Return: None
*  
*******************************************************************************/

void Init_HW(void)
{
    /*PWM Initialization*/
	PWM_Drive_Start();
    /*Enable PWM, disable PWM IO output*/
    CtrlReg_PWMOut_Write(0x01);       
   
	/*Speed Counter Initialization*/
	Counter_Spd_Start();
	
    /*Time Counter Initialization*/
    Timer_1_Start();
    Timer_1_Enable();
    
    /*For voltage sample*/
	ADC_SAR_Seq_1_Start();
    
    /*For over current protection*/
	IDAC_Iref_Start();
    IDAC_Iref_SetValue(MAX_CURR_HOMING);
    
    LPComp_OC_Start();
	
    /*UART Init*/
    UART_BCP_Start();

	isr_timeMeasurementForSpeed_Start();
	isr_timeMeasurementForSpeed_StartEx(time_isr);
	isr_timeMeasurementForSpeed_Enable();

	isr_pwm_Start();
	isr_pwm_StartEx(pwm_isr);
	isr_pwm_Enable();  
	
	isr_spd_Start(); 
	isr_spd_StartEx(speed_measure_isr);    
	isr_spd_Enable(); 	
    
	isr_oc_Start(); 
	isr_oc_StartEx(over_current_isr);    
	isr_oc_Disable();  
}

/*******************************************************************************
* Function Name: Init_UI_HW
********************************************************************************
* Summary: This function initializes hardware peripherals for user interface
*  
* Parameters: None  
*
* Return: None
*  
*******************************************************************************/
void Init_UI_HW()
{
	UpdateStatusInit();
}

/* [] END OF FILE */
