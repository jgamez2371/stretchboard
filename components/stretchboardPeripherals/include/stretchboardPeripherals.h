/*
 * stretchboardPeripherals.h
 *
 *  Created on: 10.11.2018
 *      Author: jonathan
 */

#ifndef STRETCHBOARDPERIPHERALS_H_
#define STRETCHBOARDPERIPHERALS_H_

#include "driver/ledc.h"
#include "driver/timer.h"

#define LEDC_HS_TIMER         	LEDC_TIMER_0
#define LEDC_HS_MODE          	LEDC_HIGH_SPEED_MODE
#define LEDC_HS_CH0_GPIO      	GPIO_NUM_2
#define LEDC_HS_CH0_CHANNEL   	LEDC_CHANNEL_0
#define LEDC_TIMER_RESOLUTION 	LEDC_TIMER_10_BIT
#define LEDC_DUTY_MAX			(1 << LEDC_TIMER_RESOLUTION)

#define BASS_DISABLE_PIN	GPIO_NUM_17
#define BASS_DIRECTION_PIN	GPIO_NUM_5
#define BASS_PWM_PIN		GPIO_NUM_18
#define BASS_FREQ_TIMER_RES	LEDC_TIMER_16_BIT
#define BASS_PWM_CHANNEL	LEDC_CHANNEL_2
#define BASS_PWM_TIMER		LEDC_TIMER_2
#define BASS_PWM_TIMER_RES	LEDC_TIMER_8_BIT
#define BASS_PWM_DUTY_MAX	(1 << BASS_PWM_TIMER_RES)

#define PCA9536_7_BIT_ADDRESS 0x41
#define PCA9536_I2C_WRITE_ADDRESS PCA9536_7_BIT_ADDRESS<<1
#define PCA9536_I2C_READ_ADDRESS (PCA9536_7_BIT_ADDRESS<<1)+1

typedef enum ledIntensityLevel_t
{
	INTENSITY_OFF,
	INTENSITY_LOW,
	INTENSITY_MEDIUM,
	INTENSITY_HIGH,
	INTENSITY_MAX
} programIntensityLevel_t;

extern int8_t bassIntensity;

void ledConfig();
void testLED();
void setLEDDuty(uint32_t duty);
void switchLEDOff();
void setLEDIntesity(programIntensityLevel_t intensity);

void bassConfig();
void setBassPWMDuty(uint32_t duty);
void setBassIntesity(programIntensityLevel_t intensity);
void switchBassOff();

#endif /* STRETCHBOARDPERIPHERALS_H_ */
