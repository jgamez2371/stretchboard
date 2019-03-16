#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_intr_alloc.h"
#include <driver/adc.h>
#include "driver/spi_master.h"
#include "stretchboardPeripherals.h"
#include "driver/mcpwm.h"
#include "math.h"

#include "menuLogic.h"


// SDA
#define PIN_SDA 13

// SCL
#define PIN_SCL 14

// RTOS stuff
TaskHandle_t xInputTask		= NULL;
TaskHandle_t xControlTask	= NULL;
TaskHandle_t xOutputTask	= NULL;
TaskHandle_t xBlinkTask		= NULL;
TaskHandle_t xMotorTask		= NULL;
TaskHandle_t xADCTask		= NULL;
TaskHandle_t xTickTask		= NULL;
TaskHandle_t xBassTask		= NULL;
SemaphoreHandle_t i2cSemaphore = NULL;

// Function declaration
void input_task(void *pvParameter);
void control_task(void *pvParameter);
void output_task(void *pvParameter);
void motor_task(void *pvParameter);
void tick_task(void *pvParameter);
void bass_task(void *pvParameter);
void timerISR(void *para);

void initAll();
void startProgram(programSettings_t * settings);
void stopProgram(programSettings_t *settings);

// Global variables
float signalFrequency = 20;
#define STABLE_TIME 2*60
#define STOCHASTIC_TIME 30
#define PROGRAM_PERIOD (STABLE_TIME + STOCHASTIC_TIME)

esp_err_t event_handler(void *ctx, system_event_t *event)
{
    return ESP_OK;
}

programSettings_t p1Settings =
{
	.time = SETTINGS_TDEF,
	.elapsedTime = 0,
	.intensity =  SETTINGS_INTDEF,
	.infrared = SETTINGS_INFDEF,
	.frequency = SETTINGS_FREQDEF,
	.angle = SETTINGS_ANGDEF,
	.freqArray = {16, 18, 16, 14, 12, 14, 0},
	.currentFreqIndex = 0
};
programSettings_t p2Settings =
{
	.time = SETTINGS_TDEF,
	.elapsedTime = 0,
	.intensity =  SETTINGS_INTDEF,
	.infrared = SETTINGS_INFDEF,
	.frequency = SETTINGS_FREQDEF,
	.angle = SETTINGS_ANGDEF,
	.freqArray = {22, 20, 22, 24, 26, 24, 0},
	.currentFreqIndex = 0
};
programSettings_t p3Settings =
{
	.time = SETTINGS_TDEF,
	.elapsedTime = 0,
	.intensity =  SETTINGS_INTDEF,
	.infrared = SETTINGS_INFDEF,
	.frequency = SETTINGS_FREQDEF,
	.angle = SETTINGS_ANGDEF,
	.freqArray = {40, 45, 40, 35, 0},
	.currentFreqIndex = 0
};

programSettings_t p4Settings =
{
	.time = SETTINGS_TDEF,
	.intensity =  SETTINGS_INTDEF,
	.infrared = SETTINGS_INFDEF,
	.frequency = SETTINGS_FREQDEF,
	.angle = SETTINGS_ANGDEF,
	.freqArray = {0, 0},
	.currentFreqIndex = 0
};

void app_main(void)
{
    nvs_flash_init();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );

    printf("Booo!\n");
    // i2c semaphore
    i2cSemaphore = xSemaphoreCreateMutex();
    xSemaphoreGive(i2cSemaphore);
    // Tasks creation
    // Core 1
    xTaskCreatePinnedToCore(&output_task, "output_task",
    		configMINIMAL_STACK_SIZE*5, NULL, 2, &xOutputTask, 1);
    xTaskCreatePinnedToCore(&tick_task, "tick_task",
        		configMINIMAL_STACK_SIZE*5, NULL, 3, &xTickTask, 1);
    xTaskCreatePinnedToCore(&control_task, "control_task",
        		configMINIMAL_STACK_SIZE*5, NULL, 3, &xControlTask, 1);
    xTaskCreatePinnedToCore(&motor_task, "motor_task",
    		configMINIMAL_STACK_SIZE*5, NULL, 2, &xMotorTask, 1);
    // Core 0
    xTaskCreatePinnedToCore(&bass_task, "bass_task",
    		configMINIMAL_STACK_SIZE*5, NULL, 4, &xBassTask, 0);
    // Rest
    xTaskCreate(&input_task, "input_task",
    		configMINIMAL_STACK_SIZE*5, NULL, 2, &xInputTask);
    xTaskNotify(xControlTask, EV_NONE, eSetValueWithOverwrite);
} // app_main

void input_task(void *pvParameter)
{
	uint16_t delayTime = 100;
	uint8_t keyMask;
	controlEvent_t eventToSend;
	uint32_t notificationValue;
	BaseType_t notificationSuccess = pdFALSE;

	printf("Input task entered\n\r");

	notificationSuccess = xTaskNotifyWait( 0, ULONG_MAX, &notificationValue,
			(TickType_t) 1000);
	if(notificationSuccess == pdTRUE)
	{
		while(1)
		{
			// Read keyboard
			if(xSemaphoreTake(i2cSemaphore, 250) == pdTRUE)
			{
				keyMask = readPCA9536();
				if(checkKeyAccepted(keyMask))
				{
					eventToSend = menuReactToKey(keyMask);
					xTaskNotify(xControlTask, eventToSend,
							eSetValueWithOverwrite);
					xTaskNotify(xOutputTask, EV_DISPLAY_UPDATE,
							eSetValueWithOverwrite);
				} // checkKeyAccepted(keyMask)
				xSemaphoreGive(i2cSemaphore);
			}//(xSemaphoreTake(i2cSemaphore, 250) == pdTRUE)
			else
			{
				// TODO: Resource not taken
			} //(xSemaphoreTake(i2cSemaphore, 250) == pdTRUE)
			vTaskDelay((TickType_t) delayTime);
		} // while(1)
	} // (notificationSuccess == pdTRUE)
	else
	{
		printf("Input task start timeout \r\n");
	} // !(notificationSuccess == pdTRUE)
} //input_task

typedef enum controlState_t
{
	ST_CONTROL_INIT,
	ST_CONTROL_IDLE,
	ST_CONTROL_PROGRAM
} controlState_t;

void control_task(void *pvParameter)
{
	controlState_t state = ST_CONTROL_INIT;
	controlEvent_t receivedEvent;
	BaseType_t notificationStatus;
	programSettings_t *currentProgSettings;
	int16_t elapsedTime;
	while(1)
	{
		notificationStatus = xTaskNotifyWait(0, ULONG_MAX, &receivedEvent,
				portMAX_DELAY);
		if(notificationStatus == pdTRUE)
		{
			switch(state)
			{
			case ST_CONTROL_INIT:
				// Initialize everything
				initAll();
				// Notify tasks that initialization is done
				xTaskNotify(xInputTask, EV_NONE, eNoAction);
				xTaskNotify(xOutputTask, MENU_MAIN, eSetValueWithOverwrite);
				state = ST_CONTROL_IDLE;
				break;
			case ST_CONTROL_IDLE:
				switch(receivedEvent)
				{
					case EV_START:
						currentProgSettings =
								getMenuPtr(getCurrentMenu())->settings;
						startProgram(currentProgSettings);
						state = ST_CONTROL_PROGRAM;
						break;
					default:
						break;
				} // receivedEvent
				break;
			case ST_CONTROL_PROGRAM:
				switch(receivedEvent)
				{
				case EV_STOP:
					stopProgram(currentProgSettings);
					setMenu(MENU_MAIN);
					state = ST_CONTROL_IDLE;
					break;
				case EV_TICK:
					elapsedTime = ++(currentProgSettings->elapsedTime);
					// Timeout
					if(currentProgSettings->time - elapsedTime <= 0)
					{
						stopProgram(currentProgSettings);
						setMenu(MENU_MAIN);
						state = ST_CONTROL_IDLE;
					}
					else // Normal tick
					{
						int16_t sequenceTime = elapsedTime % PROGRAM_PERIOD;
						// Stochastic vibration
						if(sequenceTime >= STABLE_TIME)
						{
							// Random between 12 and 45 Hz
							signalFrequency =  esp_random() % 34 + 12;
							//signalFrequency = 10;
						}
						else if(sequenceTime == 0)
						{
							signalFrequency = getNextFreq(currentProgSettings);
						}
					}

					break;
				case EV_UPDATE_LED:
					setLEDIntesity(currentProgSettings->infrared);
					break;
				case EV_UPDATE_INTENSITY:
					setBassIntesity(currentProgSettings->intensity);
					break;
				case EV_UPDATE_FREQUENCY:
					signalFrequency =  getNextFreq(currentProgSettings);
					break;
				default:
					break;
				} // receivedEvent
				xTaskNotify(xOutputTask, EV_DISPLAY_UPDATE,
						eSetValueWithOverwrite);
				break;
			default:
				break;
			} // state
		} // (notificationStatus == pdTRUE)
	} // while(1)
} // control_task

void output_task(void *pvParameter)
{
	uint32_t notificationValue;
	BaseType_t notificationSuccess = pdFALSE;
	while(1)
	{
		notificationSuccess = xTaskNotifyWait( 0, ULONG_MAX, &notificationValue,
				portMAX_DELAY);
		if(notificationSuccess == pdTRUE)
		{
			switch(notificationValue)
			{
				case EV_DISPLAY_UPDATE:
					menuRefresh();
					break;
				default:
					break;
			} // notificationValue

		} // (notificationSuccess == pdTRUE)
	} // while(1)
} // output_task

void tick_task(void *pvParameter)
{
	int32_t time = 0;
	char str[50];
	while(1)
	{
		int32_t time_b = time;
		uint8_t hour = time_b/3600;
		time_b -= hour*3600;
		uint8_t min = time_b/60;
		time_b -= min*60;
		uint8_t sec = time_b;
		sprintf(str, "Time: %d:%02d:%02d\r\n", hour, min, sec);
		printf(str);
		time++;
		xTaskNotify(xControlTask, EV_TICK, eSetValueWithOverwrite);
		vTaskDelay(1000);
	}
}

void bass_task(void *pvParameter)
{
	float taskPeriod = 100e-6;
	float signalAmplitude = 1;
	float phase = 0;
	uint32_t bassDutyCycle = 0;
	float signalValue = 0;
    while(1)
	{
		// Suspend this task
		if(ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100)) != 0)
		{
			// Calculate phase resolution
			if(signalFrequency < 1) signalFrequency = 1;
			float signalPeriod = 1/(float)signalFrequency;
			float phaseResolution = 2*M_PI/(signalPeriod/((float)taskPeriod));

			// Calculate PWM value
			signalAmplitude = (float)bassIntensity/(BASS_INTENSITY_MAX/10);
			signalValue = signalAmplitude*sin(phase);
			bassDutyCycle = (uint32_t)(BASS_PWM_DUTY_MAX*fabs(signalValue));
			setBassPWMDuty(bassDutyCycle);

			// Set direction according to the sign
			if(signalValue > 0) gpio_set_level(BASS_DIRECTION_PIN, 1);
			else				gpio_set_level(BASS_DIRECTION_PIN, 0);

			// Calculate phase for next cycle
			if((phase += phaseResolution) > 2*M_PI)
			{
				phase -= 2*M_PI;
			}
		}
	}
}

void initAll()
{
	initMenu(PIN_SDA, PIN_SCL, &p1Settings, &p2Settings, &p3Settings,
			&p4Settings);
	ledConfig();
	bassConfig();
	motorConfig();
	// Timer init
	timer_config_t timerConfig =
	{
		.alarm_en = true,      /*!< Timer alarm enable */
		.counter_en = true,    /*!< Counter enable */
		.intr_type = TIMER_INTR_LEVEL, /*!< Interrupt mode */
		.counter_dir = TIMER_COUNT_UP, /*!< Counter direction  */
		.auto_reload = true,   /*!< Timer auto-reload */
		.divider= 80  // 1 MHz timer
	};
	timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, 100); // Every 100 us
	timer_isr_register(TIMER_GROUP_0, TIMER_0, timerISR, NULL, ESP_INTR_FLAG_IRAM, NULL);
	timer_init(TIMER_GROUP_0, TIMER_0, &timerConfig);
}

typedef enum motorState_t
{
	ST_MOTOR_ADJUSTING,
	ST_MOTOR_STABLE
} motorState_t;

#define MOTOR_ERROR_MARGIN_LOW 2
#define MOTOR_ERROR_MARGIN_HIGH 15

#define MOTOR_ERROR_THRESHOLD_1 10
#define MOTOR_ERROR_THRESHOLD_2 20

void motor_task(void *pvParameter)
{
	motorState_t state = ST_MOTOR_ADJUSTING;
	// ADC config
    int read_raw;
    int target;
    int angleError;
    adc2_config_channel_atten( ADC2_CHANNEL_0, ADC_ATTEN_DB_11);
    printf("Motor task on core %d\r\n", xPortGetCoreID());

	while(1)
	{
		target = ((p4Settings.angle)/10)*51;

		angleError = 0;
	    esp_err_t r = adc2_get_raw( ADC2_CHANNEL_0, ADC_WIDTH_BIT_9, &read_raw);
	    if ( r == ESP_OK )
	    {
	        //printf("%d\n", read_raw );
	        angleError = target - read_raw;
	    } else if ( r == ESP_ERR_TIMEOUT )
	    {
	        printf("ADC2 used by Wi-Fi.\n");
	    }
	    switch(state)
	    {
	    	case ST_MOTOR_ADJUSTING:
	    		if(abs(angleError) > MOTOR_ERROR_MARGIN_LOW)
	    		{
	    			// Switch on
	    			gpio_set_level(MOTOR_ENABLE_PIN, 0); // Enable active low
	    			// TODO: set PWM duty cycle
	    			//gpio_set_level(MOTOR_PWM_PIN, 1); // PWM
	    			if(abs(angleError) < MOTOR_ERROR_THRESHOLD_1)
	    			{
	    				setMotorPWMDuty(LEDC_DUTY_MAX/16);
	    			}
	    			else if(abs(angleError) < MOTOR_ERROR_THRESHOLD_2)
	    			{
	    				setMotorPWMDuty(LEDC_DUTY_MAX/4);
	    			}
	    			else
	    			{
	    				setMotorPWMDuty(LEDC_DUTY_MAX);
	    			}
	    			// Set direction
	    			if(angleError > 0)
	    			{
	    				gpio_set_level(MOTOR_DIRECTION_PIN, 0); // Increase
	    			}
	    			else
	    			{
	    				gpio_set_level(MOTOR_DIRECTION_PIN, 1); // Decrease
	    			}
	    		}
	    		else
	    		{
	    			// Switch off
	    			gpio_set_level(MOTOR_ENABLE_PIN, 1); // Enable active low
	    			setMotorPWMDuty(LEDC_DUTY_MAX);
	    			//gpio_set_level(MOTOR_PWM_PIN, 0); // PWM
	    			state = ST_MOTOR_STABLE;
	    		}
	    		break;
	    	// ST_MOTOR_ADJUSTING
	    	case ST_MOTOR_STABLE:
	    		if(abs(angleError) > MOTOR_ERROR_MARGIN_HIGH)
	    		{
	    			state = ST_MOTOR_ADJUSTING;
	    		}
	    		break;
	    	// ST_MOTOR_STABLE
	    } // state
	    vTaskDelay(33);
	} // while(1)
} // motor_task

void startProgram(programSettings_t * settings)
{
	setLEDIntesity((uint8_t)settings->infrared);
	setBassIntesity((uint8_t)settings->intensity);
	signalFrequency = getNextFreq(settings);
}

void stopProgram(programSettings_t *settings)
{
	// Reset timer
	settings->time = SETTINGS_TDEF;
	settings->elapsedTime = 0;
	settings->currentFreqIndex = 0;
	switchLEDOff();
	// Switch bass off
	switchBassOff();
}

void IRAM_ATTR timerISR(void *para)
{
	// Execute bass task
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	vTaskNotifyGiveFromISR(xBassTask, &xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR();
	// Re-enable alarm
	TIMERG0.hw_timer[0].config.alarm_en = TIMER_ALARM_EN;
	// Clear interrupt
	TIMERG0.int_clr_timers.t0 = 1;
}
