/*
 * streatchboardMenu.c
 *
 *  Created on: 04.11.2018
 *      Author: jonathan
 */

#include "simpleMenu.h"

extern SemaphoreHandle_t i2cSemaphore;
static u8g2_t u8g2; // a structure which will contain all the data for one display

void task_test_SSD1306i2c(void *ignore)
{
	menu_displayInit(13, 14);

	menuBase_t testMenu;
	testMenu.selectedRow = 0;
	testMenu.title = 0;
	menu_setLine(&testMenu, 0, "  Stretch4Me  ");
	menu_setLine(&testMenu, 1, "P1 Muskeln");
	menu_setLine(&testMenu, 2, "P2 Faszien");
	menu_setLine(&testMenu, 3, "P3 Schmerzen");
	menu_draw(&testMenu);
//	for(int8_t i = 0; i < 5; i++)
//	{
//		for(int8_t j = 0; j < 6; j++)
//		{
//			menu_selectLine(&testMenu, LINE_NEXT);
//			menu_draw(&testMenu);
//			vTaskDelay(250);
//		}
//		for(int8_t j = 0; j < 6; j++)
//		{
//			menu_selectLine(&testMenu, LINE_PREVIOUS);
//			menu_draw(&testMenu);
//			vTaskDelay(250);
//		}
//	}
} //task_test_SSD1306i2c

void menu_displayInit(uint8_t sda, uint8_t scl)
{
	if(xSemaphoreTake(i2cSemaphore, 500) == pdTRUE)
	{
		u8g2_esp32_hal_t u8g2_esp32_hal = U8G2_ESP32_HAL_DEFAULT;
		u8g2_esp32_hal.sda   = sda;
		u8g2_esp32_hal.scl  = scl;
		u8g2_esp32_hal_init(u8g2_esp32_hal);

		u8g2_Setup_ssd1305_i2c_128x64_adafruit_f
		(
			&u8g2,
			U8G2_R2,
			u8g2_esp32_i2c_byte_cb,
			u8g2_esp32_gpio_and_delay_cb
		);
		u8x8_SetI2CAddress(&u8g2.u8x8,0x78);
		u8g2_InitDisplay(&u8g2);
		u8g2_ClearBuffer(&u8g2);
		u8g2_SendBuffer(&u8g2);
		u8g2_SetPowerSave(&u8g2, 0); // wake up display
		// Return semaphore
		xSemaphoreGive(i2cSemaphore);
	}
	else
	{
		// TODO: Semaphore not taken
	}
}

int8_t menu_selectLine(menuBase_t *menu, int8_t selection)
{
	menu->selectedRow = selection;
//	switch(selection)
//	{
//		case LINE_NEXT:
//			if(++menu->selectedRow > MENU_ROW_COUNT - 1)
//			{
//				menu->selectedRow = 0;
//			}
//			break;
//		case LINE_PREVIOUS:
//			if(--menu->selectedRow < 0)
//			{
//				menu->selectedRow = MENU_ROW_COUNT - 1;
//			}
//			break;
//	} // selection
	return menu->selectedRow;
} // menu_selectLine

void menu_setLine(menuBase_t *menu, int8_t line, const char *str)
{
	strncpy(menu->contents[line], str, MENU_COL_COUNT - 1);
	menu->contents[line][MENU_COL_COUNT - 1] = '\0';
} // menu_setLine

void menu_fill(menuBase_t *menu, int8_t selectedRow, int8_t title,
		const char *row0Str, const char *row1Str, const char *row2Str,
		const char *row3Str)
{
	menu->selectedRow = selectedRow;
	menu->title = title;
	menu_setLine(menu, 0, row0Str);
	menu_setLine(menu, 1, row1Str);
	menu_setLine(menu, 2, row2Str);
	menu_setLine(menu, 3, row3Str);
} // menu_fill

void menu_draw(menuBase_t *menu)
{
	if(xSemaphoreTake(i2cSemaphore, 500) == pdTRUE)
	{
		u8g2_ClearBuffer(&u8g2);
		for(uint8_t currentRow = 0; currentRow < MENU_ROW_COUNT; currentRow++)
		{
			// Check for title
			if(menu->title == currentRow)
			{
				u8g2_SetFont(&u8g2, u8g2_font_9x15B_mf);//u8g2_font_t0_15b_mf);
			} // (menu->title == currentRow)
			else // !(menu->title == currentRow)
			{
				u8g2_SetFont(&u8g2, u8g2_font_9x15_me);
			} // !(menu->title == currentRow)
			// Check for selected row
			if(menu->selectedRow == currentRow)
			{
				u8g2_SetDrawColor(&u8g2, 0);
			} //(menu->currentRow == currentRow)
			else // !(menu->currentRow == currentRow)
			{
				u8g2_SetDrawColor(&u8g2, 1);
			} // !(menu->currentRow == currentRow)
			u8g2_DrawStr(&u8g2, 0, 16*(currentRow+1) - 3, menu->contents[currentRow]);
		} // currentRow
		u8g2_SendBuffer(&u8g2);
		xSemaphoreGive(i2cSemaphore);
	} //
	else
	{
		// TODO: Semaphore not taken
	}
} //menu_draw
