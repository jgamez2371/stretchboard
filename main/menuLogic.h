/*
 * menuLogic.h
 *
 *  Created on: 05.11.2018
 *      Author: jonathan
 */

#ifndef MENULOGIC_H_
#define MENULOGIC_H_

#include "simpleMenu.h"
#include "stretchboardPeripherals.h"

typedef enum controlEvent_t
{
	EV_NONE 				= 0x00,
	EV_START				= 0x01 << 0,
	EV_STOP					= 0x01 << 1,
	EV_TICK					= 0x01 << 2,
	EV_UPDATE_LED			= 0x01 << 3,
	EV_UPDATE_INTENSITY		= 0x01 << 4,
	EV_UPDATE_FREQUENCY		= 0x01 << 5,
	EV_UPDATE_ANGLE			= 0x01 << 6
} controlEvent_t;

typedef enum outputEvent_t
{
	EV_DISPLAY_UPDATE
} outputEvent_t;

typedef struct programSettings_t
{
	int16_t time;
	int16_t elapsedTime;
	int8_t intensity;
	int8_t infrared;
	int8_t frequency;
	int8_t angle;
	int8_t freqArray[10];
	int8_t currentFreqIndex;
} programSettings_t;

typedef struct programMenu_t
{
	menuBase_t menu;
	programSettings_t *settings;
} programMenu_t;

#define SETTINGS_TMAX		99*60
#define SETTINGS_TMIN		60
#define SETTINGS_TDEF		10*60

#define SETTINGS_INTDEF		INTENSITY_LOW
#define SETTINGS_INFDEF		INTENSITY_LOW

#define SETTINGS_FREQMAX	45
#define SETTINGS_FREQMIN	12
#define SETTINGS_FREQDEF	20
#define SETTINGS_ANGMAX		100
#define SETTINGS_ANGMIN		0
#define SETTINGS_ANGDEF		20

typedef enum stretchboardMenu_t
{
	MENU_MAIN,
	MENU_P1,
	MENU_P2,
	MENU_P3,
	MENU_P4_1,
	MENU_P4_2,
	MENU_MAX
} stretchboardMenu_t;



typedef enum scrollSelection_t
{
	SCROLL_DOWN,
	SCROLL_UP
} scrollSelection_t;

typedef enum keyState_t
{
	ST_KEY_RELEASED,
	ST_KEY_PRESSED_ONCE,
	ST_KEY_COUNTING,
	ST_KEY_PRESSED_MULTIPLE
} keyState_t;

typedef enum pressedKey
{
	KEY_NONE	= 0xFF,
	KEY_UP		= 0xFF - (0x01 << 0),
	KEY_DOWN	= 0xFF - (0x01 << 1),
	KEY_LEFT	= 0xFF - (0x01 << 2),
	KEY_RIGHT	= 0xFF - (0x01 << 3)
} pressedKey_t;
extern stretchboardMenu_t currentMenu;

void initMenu(uint8_t sda, uint8_t scl, programSettings_t *p1Settings,
		programSettings_t *p2Settings, programSettings_t *p3Settings,
		programSettings_t *p4Settings);
void testAllMenus();
void menuDraw(stretchboardMenu_t menu);
void menuRefresh();
void menuScroll(scrollSelection_t direction);
void setMenu(stretchboardMenu_t menu);
programMenu_t *getMenuPtr(stretchboardMenu_t menu);
controlEvent_t menuReactToKey(uint8_t keyMask);
controlEvent_t mainMenuReactToKey(uint8_t keyMask);
controlEvent_t programMenuReactToKey(stretchboardMenu_t menu, uint8_t keyMask);
controlEvent_t p4_1ReactToKey(uint8_t keyMask);
controlEvent_t p4_2ReactToKey(uint8_t keyMask);
stretchboardMenu_t getCurrentMenu();
void menuUpdateVariables(stretchboardMenu_t menu);
uint8_t checkKeyAccepted(uint8_t keyMask);
int8_t getNextFreq(programSettings_t* settings);

#endif /* MENULOGIC_H_ */
