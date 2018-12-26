/*
 * stretchboardPeripherals.c
 *
 *  Created on: 10.11.2018
 *      Author: jonathan
 */

#include "stretchboardPeripherals.h"

int8_t bassIntensity;

static ledc_timer_config_t ledc_timer =
{
    .duty_resolution = LEDC_TIMER_RESOLUTION, // resolution of PWM duty
    .freq_hz = 1000,                      // frequency of PWM signal
    .speed_mode = LEDC_HS_MODE,           // timer mode
    .timer_num = LEDC_HS_TIMER            // timer index
};

static ledc_channel_config_t ledc_channel =
{
	.channel    = LEDC_HS_CH0_CHANNEL,
	.duty       = 0,
	.gpio_num   = LEDC_HS_CH0_GPIO,
	.speed_mode = LEDC_HS_MODE,
	.timer_sel  = LEDC_HS_TIMER
};


void ledConfig()
{
	    ledc_timer_config(&ledc_timer);
	    ledc_channel_config(&ledc_channel);
	    switchLEDOff();
}

void testLED()
{
	uint32_t step = LEDC_DUTY_MAX/16;
	uint32_t dutyMax = LEDC_DUTY_MAX/2;
	static uint32_t duty = 0;
    if((duty+= step) > dutyMax)
    {
    	duty = 0;
    	switchLEDOff();
    }
    else
    {
    	setLEDDuty(duty);
    }
    setLEDDuty(duty);
	printf("LED duty cycle: %d\r\n", ledc_channel.duty);
}

void setLEDDuty(uint32_t duty)
{
	ledc_channel.duty = duty;
	ledc_channel_config(&ledc_channel);
}

void switchLEDOff()
{
	ledc_stop(LEDC_HS_MODE, LEDC_HS_CH0_CHANNEL, 0);
}

void setLEDIntesity(programIntensityLevel_t intensity)
{
	if(intensity >= INTENSITY_MAX)
	{
		intensity = INTENSITY_HIGH;
	}
	setLEDDuty(intensity*(LEDC_DUTY_MAX/INTENSITY_HIGH));
}


// TODO: Delete
static ledc_timer_config_t bassFreqTimer =
{
    .duty_resolution = BASS_FREQ_TIMER_RES, // resolution of PWM duty
    .freq_hz = 20,                        // frequency of PWM signal
    .speed_mode = LEDC_HIGH_SPEED_MODE,   // timer mode
    .timer_num = LEDC_TIMER_1             // timer index
};

// TODO: Delete
static ledc_channel_config_t bassFreqChannel =
{
	.channel    = LEDC_CHANNEL_1,
	.duty       = (1 << BASS_FREQ_TIMER_RES)/2, // 50%
	.gpio_num   = BASS_DIRECTION_PIN,
	.speed_mode = LEDC_HIGH_SPEED_MODE,
	.timer_sel  = LEDC_TIMER_1
};

static ledc_timer_config_t bassPWMTimer =
{
    .duty_resolution = BASS_PWM_TIMER_RES, // resolution of PWM duty
    .freq_hz = 20000,                        // frequency of PWM signal
    .speed_mode = LEDC_HIGH_SPEED_MODE,   // timer mode
    .timer_num = BASS_PWM_TIMER             // timer index
};

static ledc_channel_config_t bassPWMChannel =
{
	.channel    = BASS_PWM_CHANNEL,
	.duty       = 0,
	.gpio_num   = BASS_PWM_PIN,
	.speed_mode = LEDC_HIGH_SPEED_MODE,
	.timer_sel  = BASS_PWM_TIMER
};

void bassConfig()
{
	// TODO: Delete
    //ledc_timer_config(&bassFreqTimer);
    //ledc_channel_config(&bassFreqChannel);
    ledc_channel_config(&bassPWMChannel);
    ledc_timer_config(&bassPWMTimer);
	const gpio_config_t bassShakerGPIOConfig =
    {
		.pin_bit_mask = (1<<BASS_DISABLE_PIN)|(1 << BASS_DIRECTION_PIN),
		.mode = GPIO_MODE_INPUT_OUTPUT,
		.pull_up_en = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&bassShakerGPIOConfig);
    gpio_set_level(BASS_DISABLE_PIN, 1);
    gpio_set_level(BASS_DIRECTION_PIN, 1);
}

void setBassIntesity(programIntensityLevel_t intensity)
{
	gpio_set_level(BASS_DISABLE_PIN, 0);
	if(intensity >= INTENSITY_MAX)
	{
		intensity = INTENSITY_HIGH;
	}
	bassIntensity = intensity;
	//setBassPWMDuty(intensity*(BASS_PWM_DUTY_MAX/INTENSITY_HIGH));
}

void setBassPWMDuty(uint32_t duty)
{
	bassPWMChannel.duty= duty;
	ledc_channel_config(&bassPWMChannel);
}

void switchBassOff()
{
	gpio_set_level(BASS_DISABLE_PIN, 1);
	ledc_stop(LEDC_HIGH_SPEED_MODE, BASS_PWM_CHANNEL, 0);
}
