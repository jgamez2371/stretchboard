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


// SDA - GPIO21
#define PIN_SDA 13

// SCL - GPIO22
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
void adc_task(void *pvParameter);
void tick_task(void *pvParameter);
void bass_task(void *pvParameter);
void timerISR(void *para);

void initAll();
void startProgram(programSettings_t * settings);
void stopProgram(programSettings_t *settings);

// Global variables
float signalFrequency = 20;
#define STABLE_TIME 10//2*60;
#define STOCHASTIC_TIME 5//30;
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
	.angle = SETTINGS_ANGDEF
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
    // Core 0
    xTaskCreatePinnedToCore(&output_task, "output_task",
    		configMINIMAL_STACK_SIZE*5, NULL, 2, &xOutputTask, 0);
    // Core 1
    xTaskCreatePinnedToCore(&bass_task, "bass_task",
    		configMINIMAL_STACK_SIZE*5, NULL, 4, &xBassTask, 1);
    // Rest
    xTaskCreate(&input_task, "input_task",
    		configMINIMAL_STACK_SIZE*5, NULL, 2, &xInputTask);
    xTaskCreate(&control_task, "control_task",
    		configMINIMAL_STACK_SIZE*5, NULL, 2, &xControlTask);
    xTaskCreate(&tick_task, "tick_task",
    		configMINIMAL_STACK_SIZE*5, NULL, 2, &xTickTask);
    xTaskNotify(xControlTask, EV_NONE, eSetValueWithOverwrite);
    xTaskCreate(&motor_task, "motor_task",
    		configMINIMAL_STACK_SIZE*5, NULL, 2, &xMotorTask);
    xTaskCreate(&adc_task, "adc_task",
    		configMINIMAL_STACK_SIZE*5, NULL, 2, &xADCTask);
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
						if(sequenceTime > STABLE_TIME)
						{
							// Random between 12 and 45 Hz
							//signalFrequency =  esp_random() % 34 + 12;
							signalFrequency = 10;
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
				case EVENT_UPDATE_INTENSITY:
					setBassIntesity(currentProgSettings->intensity);
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
	float taskPeriod = 100/1e6;
	float signalAmplitude = 1;
	float phase = 0;
	uint32_t bassDutyCycle = 0;
	float signalValue = 0;

    const gpio_config_t testGPIO =
    {
		.pin_bit_mask = (1<<GPIO_NUM_21),
		.mode = GPIO_MODE_OUTPUT,
		.pull_up_en = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&testGPIO);
    gpio_set_level(GPIO_NUM_21, 0); // Disable

    while(1)
	{
		// Suspend this task
		if(ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100)) != 0)
		{
			gpio_set_level(GPIO_NUM_21, 1); // Enable

			// Calculate phase resolution
			float signalPeriod = 1/(float)signalFrequency;
			float phaseResolution = 2*M_PI/(signalPeriod/((float)taskPeriod));

			// Calculate PWM value
			signalAmplitude = (float)bassIntensity/INTENSITY_HIGH;
			signalValue = signalAmplitude*sin(phase);
			bassDutyCycle = BASS_PWM_DUTY_MAX*fabs(signalValue);
			setBassPWMDuty(bassDutyCycle);

			// Set direction according to the sign
			if(signalValue > 0) gpio_set_level(BASS_DIRECTION_PIN, 1);
			else				gpio_set_level(BASS_DIRECTION_PIN, 0);

			// Calculate phase for next cycle
			if((phase += phaseResolution) > 2*M_PI)
			{
				phase -= 2*M_PI;
			}
			gpio_set_level(GPIO_NUM_21, 0); // Disable
		}
	}
}

void initAll()
{
	initMenu(PIN_SDA, PIN_SCL, &p1Settings, &p2Settings, &p3Settings,
			&p4Settings);
	ledConfig();
	bassConfig();
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

void adc_task(void *pvParameter)
{
	// SPI config
	spi_device_handle_t spiDevice;
	const spi_bus_config_t spiConfig =
	{
	    GPIO_NUM_34,                ///< GPIO pin for Master Out Slave In (=spi_d) signal, or -1 if not used.
	    GPIO_NUM_35,                ///< GPIO pin for Master In Slave Out (=spi_q) signal, or -1 if not used.
	    GPIO_NUM_32,                ///< GPIO pin for Spi CLocK signal, or -1 if not used.
	    -1,              ///< GPIO pin for WP (Write Protect) signal which is used as D2 in 4-bit communication modes, or -1 if not used.
	    -1,              ///< GPIO pin for HD (HolD) signal which is used as D3 in 4-bit communication modes, or -1 if not used.
	    0,            ///< Maximum transfer size, in bytes. Defaults to 4094 if 0.
	    0                 ///< Abilities of bus to be checked by the driver. Or-ed value of ``SPICOMMON_BUSFLAG_*`` flags.
	};
//	spi_bus_initialize(HSPI_HOST, &spiConfig, 0);
//	spi_bus_add_device(spi_host_device_t host, const spi_device_interface_config_t *dev_config, spi_device_handle_t *handle)
	// ADC config
    int read_raw;
    adc2_config_channel_atten( ADC2_CHANNEL_0, ADC_ATTEN_0db );
	while(1)
	{
	    esp_err_t r = adc2_get_raw( ADC2_CHANNEL_0, ADC_WIDTH_12Bit, &read_raw);
	    if ( r == ESP_OK ) {
	        //printf("%d\n", read_raw );
	    } else if ( r == ESP_ERR_TIMEOUT ) {
	        printf("ADC2 used by Wi-Fi.\n");
	    }
	    vTaskDelay(250);
	} // while(1)
} // adc_task

void motor_task(void *pvParameter)
{
    const gpio_config_t bassShakerGPIOConfig =
    {
		.pin_bit_mask = (1<<GPIO_NUM_26) | (1<<GPIO_NUM_27),
		.mode = GPIO_MODE_OUTPUT,
		.pull_up_en = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&bassShakerGPIOConfig);

    gpio_set_level(GPIO_NUM_26, 0); // Disable
    //gpio_set_level(GPIO_NUM_9, 1); // PWM

    while (true)
    {
    	gpio_set_level(GPIO_NUM_27, 0); // Direction
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        gpio_set_level(GPIO_NUM_27, 1); // Direction
		vTaskDelay(5000 / portTICK_PERIOD_MS);
    } // while(true)
} //motor_task

void startProgram(programSettings_t * settings)
{
	setLEDIntesity((programIntensityLevel_t)settings->infrared);
	setBassIntesity((programIntensityLevel_t)settings->intensity);
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
