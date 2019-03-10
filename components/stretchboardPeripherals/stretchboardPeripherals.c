/*
 * stretchboardPeripherals.c
 *
 *  Created on: 10.11.2018
 *      Author: jonathan
 */

#include "stretchboardPeripherals.h"

int8_t bassIntensity;

// LED PWM configuration variables
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

static ledc_channel_config_t motorPWMChannel =
{
	.channel    = MOTOR_PWM_CHANNEL,
	.duty       = 0,
	.gpio_num   = MOTOR_PWM_PIN,
	.speed_mode = LEDC_HS_MODE,
	.timer_sel  = LEDC_HS_TIMER
};

// Bass PWM configuration variables
static ledc_timer_config_t bassPWMTimer =
{
    .duty_resolution = BASS_PWM_TIMER_RES, // resolution of PWM duty
    .freq_hz = BASS_PWM_FREQ,              // frequency of PWM signal
    .speed_mode = LEDC_HIGH_SPEED_MODE,    // timer mode
    .timer_num = BASS_PWM_TIMER            // timer index
};

static ledc_channel_config_t bassPWMChannel =
{
	.channel    = BASS_PWM_CHANNEL,
	.duty       = 0,
	.gpio_num   = BASS_PWM_PIN,
	.speed_mode = LEDC_HIGH_SPEED_MODE,
	.timer_sel  = BASS_PWM_TIMER
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

void setLEDIntesity(uint8_t intensity)
{
	if(intensity >= LED_INTENSITY_MAX)
	{
		intensity = LED_INTENSITY_MAX;
	}
	setLEDDuty(intensity*(LEDC_DUTY_MAX/LED_INTENSITY_MAX));
}

void motorConfig()
{
	ledc_channel_config(&motorPWMChannel);
	const gpio_config_t motorGPIOConfig =
	{
		.pin_bit_mask = (1<<MOTOR_ENABLE_PIN) | (1<<MOTOR_DIRECTION_PIN)
			| (1<<MOTOR_PWM_PIN),
		.mode = GPIO_MODE_OUTPUT,
		.pull_up_en = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type = GPIO_INTR_DISABLE
	};
	gpio_config(&motorGPIOConfig);
	gpio_set_level(MOTOR_ENABLE_PIN, 1); // Enable active low
} //motorConfig

void setMotorPWMDuty(uint32_t duty)
{
	motorPWMChannel.duty= duty;
	ledc_channel_config(&motorPWMChannel);
} // setMotorPWMDuty

void bassConfig()
{
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

void setBassIntesity(uint8_t intensity)
{
	gpio_set_level(BASS_DISABLE_PIN, 0);
	if(intensity >= BASS_INTENSITY_MAX)
	{
		intensity = BASS_INTENSITY_MAX;
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

uint8_t readPCA9536()
{
	uint8_t slaveData;
	i2c_cmd_handle_t i2cHandle = i2c_cmd_link_create();
	// 1. Start condition
	i2c_master_start(i2cHandle);
	// 2. Slave address write
	i2c_master_write_byte(i2cHandle, PCA9536_I2C_WRITE_ADDRESS, 0x1);
	// 3. CMD
	i2c_master_write_byte(i2cHandle, 0x00, 0x1);
	// 4. Restart
	i2c_master_start(i2cHandle);
	// 5. Slave address read
	i2c_master_write_byte(i2cHandle, PCA9536_I2C_READ_ADDRESS, 0x1);
	// 6. Read 1 byte NAK
	i2c_master_read_byte(i2cHandle, &slaveData,I2C_MASTER_LAST_NACK);
	// 7. Stop condition
	i2c_master_stop(i2cHandle);
	// 8. Send all the stuff
	i2c_master_cmd_begin(I2C_NUM_1, i2cHandle, (TickType_t)25);
	// 9. Delete the thing
	i2c_cmd_link_delete(i2cHandle);
	return slaveData;
} //readPCA9536
