/*
 * stretchboardMenu.h
 *
 *  Created on: 04.11.2018
 *      Author: jonathan
 */

#ifndef STRETCHBOARDMENU_H_
#define STRETCHBOARDMENU_H_

#include "u8g2_esp32_hal.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "driver/i2c.h"

#define MENU_ROW_COUNT 4
#define MENU_COL_COUNT 15

typedef struct menuBase_t
{
	char contents[MENU_ROW_COUNT][MENU_COL_COUNT];
	int8_t title;
	int8_t selectedRow;
} menuBase_t;

void menu_displayInit(uint8_t sda, uint8_t scl);
void task_test_SSD1306i2c(void *ignore);
int8_t menu_selectLine(menuBase_t *menu, int8_t selection);
void menu_setLine(menuBase_t *menu, int8_t line, const char *str);
void menu_fill(menuBase_t *menu, int8_t selectedRow, int8_t title,
		const char *row0Str, const char *row1Str, const char *row2Str,
		const char *row3Str);
void menu_draw(menuBase_t *menu);


#endif /* STRETCHBOARDMENU_H_ */
