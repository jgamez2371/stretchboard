/*
 * menuLogic.c
 *
 *  Created on: 05.11.2018
 *      Author: jonathan
 */

#include "menuLogic.h"

stretchboardMenu_t currentMenu = MENU_MAIN;
static programMenu_t mainMenu, menuP1, menuP2, menuP3, menuP4_1, menuP4_2;


const char mainTitleStr[]			= "  Stretch4Me  ";
const char P1GeStr[]				= "P1 Muskeln";
const char P1Str[] 					= "P1 Muscle";
const char P2GeStr[] 				= "P2 Faszien";
const char P2Str[] 					= "P2 Fascia";
const char P3GeStr[] 				= "P3 Schmerzen";
const char P3Str[] 					= "P3 Pain";
const char P4Str[] 					= "Individual";
const char timeRowTemplate[] 		= "Time %02d:%02d";
const char intensityRowTempalte[]	= "Intensity %d";
const char infraredRowTemplate[] 	= "Infrared %d";
const char frequencyRowTemplate[] 	= "Frequency %d";
const char angleRowTemplate[] 		= "Angle %d%%";
const char emptyRow[] = " ";

stretchboardMenu_t getCurrentMenu()
{
	return currentMenu;
} // getCurrentMenu()

void setMenu(stretchboardMenu_t menu)
{
	currentMenu = menu;
} // setMenu()

void initMenu(uint8_t sda, uint8_t scl, programSettings_t *p1Settings,
		programSettings_t *p2Settings, programSettings_t *p3Settings,
		programSettings_t *p4Settings)
{
	menu_displayInit(sda, scl);
	// Main Menu
	menu_fill(&mainMenu.menu, 0, 0, mainTitleStr, P1GeStr, P2GeStr, P3GeStr);
	mainMenu.settings = NULL;
	// Menu P1
	menu_fill(&menuP1.menu, 0, 0, P1Str, timeRowTemplate, intensityRowTempalte,
			infraredRowTemplate);
	menuP1.settings = p1Settings;
	// Menu P2
	menu_fill(&menuP2.menu, 0, 0, P2Str, timeRowTemplate, intensityRowTempalte,
			infraredRowTemplate);
	menuP2.settings = p2Settings;
	// Menu P3
	menu_fill(&menuP3.menu, 0, 0, P3Str, timeRowTemplate, intensityRowTempalte,
			infraredRowTemplate);
	menuP3.settings = p3Settings;
	// Menu P4.1
	menu_fill(&menuP4_1.menu, 0, 0, P4Str, emptyRow, timeRowTemplate, emptyRow);
	menuP4_1.settings = p4Settings;
	// Menu P4.2
	menu_fill(&menuP4_2.menu, 0, -1, intensityRowTempalte, infraredRowTemplate,
			frequencyRowTemplate, angleRowTemplate);
	menuP4_2.settings = p4Settings;
	currentMenu = MENU_MAIN;
	menuDraw(currentMenu);
} //initMenu()


void testAllMenus()
{
	uint8_t i = 0;
	while(1)
	{
		//printf("Output Task \r\n");
		currentMenu = i++ % MENU_MAX;
		menuDraw(currentMenu);
		vTaskDelay(5000);
	} // for i
} // testAlleMenus

programMenu_t *getMenuPtr(stretchboardMenu_t menu)
{
	programMenu_t *menuPtr;
	switch(menu)
	{
		case MENU_MAIN:	menuPtr = &mainMenu; break;
		case MENU_P1:	menuPtr = &menuP1; break;
		case MENU_P2:	menuPtr = &menuP2; break;
		case MENU_P3:	menuPtr = &menuP3; break;
		case MENU_P4_1: menuPtr = &menuP4_1; break;
		case MENU_P4_2: menuPtr = &menuP4_2; break;
		default:		menuPtr = &mainMenu; break;
	}
	return menuPtr;
}

void menuDraw(stretchboardMenu_t menu)
{
	currentMenu = menu;
	menuUpdateVariables(menu);
	menu_draw(&getMenuPtr(menu)->menu);
}

void menuRefresh()
{
	menuDraw(currentMenu);
}

void menuScroll(scrollSelection_t direction)
{
	menuBase_t *menu = &getMenuPtr(currentMenu)->menu;
	switch(direction)
	{
		case SCROLL_DOWN:
			if(++menu->selectedRow > MENU_ROW_COUNT - 1)
			{
				menu->selectedRow = 0;
			}
			break;
		case SCROLL_UP:
			if(--menu->selectedRow < 0)
			{
				menu->selectedRow = MENU_ROW_COUNT - 1;
			}
			break;
	} // selection
} // menuScroll

void menuUpdateVariables(stretchboardMenu_t menu)
{
	programMenu_t *menuPtr = getMenuPtr(menu);
	char tempString[50];
	if(menuPtr->settings != NULL)
	{
		uint16_t remainingTime;
		uint8_t min;
		uint8_t sec;
		// Calculate time
		remainingTime = menuPtr->settings->time -
				menuPtr->settings->elapsedTime;
		min = remainingTime / 60;
		sec = remainingTime % 60;
		switch(menu)
		{
			case MENU_P1:
			case MENU_P2:
			case MENU_P3:
				// Row 1: Time
				sprintf(tempString, timeRowTemplate, min, sec);
				menu_setLine(&menuPtr->menu, 1, tempString);
				// Row 2: Intensity
				sprintf(tempString, intensityRowTempalte,
						menuPtr->settings->intensity);
				menu_setLine(&menuPtr->menu, 2, tempString);
				// Row 3: Infrared
				sprintf(tempString, infraredRowTemplate,
						menuPtr->settings->infrared);
				menu_setLine(&menuPtr->menu, 3, tempString);
				break;
			case MENU_P4_1:
				sprintf(tempString, timeRowTemplate, min, sec);
				menu_setLine(&menuPtr->menu, 2, tempString);
				break;
			case MENU_P4_2:
				// Row 0: Intensity
				sprintf(tempString, intensityRowTempalte,
						menuPtr->settings->intensity);
				menu_setLine(&menuPtr->menu, 0, tempString);
				// Row 1: Infrared
				sprintf(tempString, infraredRowTemplate,
						menuPtr->settings->infrared);
				menu_setLine(&menuPtr->menu, 1, tempString);
				// Row 2: Frequency
				sprintf(tempString, frequencyRowTemplate,
						menuPtr->settings->frequency);
				menu_setLine(&menuPtr->menu, 2, tempString);
				// Row 3: Angle
				sprintf(tempString, angleRowTemplate,
						menuPtr->settings->angle);
				menu_setLine(&menuPtr->menu, 3, tempString);
				break;
			default:
				break;
		} // menu

	} // (menuPtr->settings != NULL)
} // menuUpdateVariables

controlEvent_t menuReactToKey(uint8_t keyMask)
{
	controlEvent_t event = EV_NONE;
	switch(currentMenu)
	{
		case MENU_MAIN:
			event = mainMenuReactToKey(keyMask);
			break;
		case MENU_P1:
		case MENU_P2:
		case MENU_P3:
			event = programMenuReactToKey(currentMenu, keyMask);
			break;
		case MENU_P4_1:
			event = p4_1ReactToKey(keyMask);
			break;
		case MENU_P4_2:
			event = p4_2ReactToKey(keyMask);
			break;
		default:
			setMenu(MENU_MAIN);
	} // currentMenu
	return event;
} // menuReactToKey

controlEvent_t mainMenuReactToKey(uint8_t keyMask)
{
	programMenu_t *menuPtr = getMenuPtr(MENU_MAIN);
	controlEvent_t event = EV_NONE;
	switch(keyMask)
	{
		case KEY_DOWN:
			menuScroll(SCROLL_DOWN);
			break;
		case KEY_UP:
			menuScroll(SCROLL_UP);
			break;
		case KEY_RIGHT:
			switch(menuPtr->menu.selectedRow)
			{
				case 1: // P1
					setMenu(MENU_P1);
					break;
				case 2: // P2
					setMenu(MENU_P2);
					break;
				case 3: // P3
					setMenu(MENU_P3);
					break;
			} // menuPtr->menu.selectedRow
			break;
		case (KEY_LEFT & KEY_RIGHT):
			setMenu(MENU_P4_1);
			break;
		default:
			break;
	} // keyMask
	return event;
} // mainMenuReactToKey

controlEvent_t programMenuReactToKey(stretchboardMenu_t menu, uint8_t keyMask)
{
	programMenu_t *menuPtr = getMenuPtr(menu);
	controlEvent_t event = EV_NONE;
	int8_t currentRow = menuPtr->menu.selectedRow;
	int16_t programTime = menuPtr->settings->time;
	int16_t elapsedTime = menuPtr->settings->elapsedTime;
	switch(keyMask)
	{
		case KEY_DOWN:
			menuScroll(SCROLL_DOWN);
			break;
		case KEY_UP:
			menuScroll(SCROLL_UP);
			break;
		case KEY_LEFT:
			switch(currentRow)
			{
				case 0: // Title
					event = EV_STOP;
					setMenu(MENU_MAIN);
					break;
				case 1: // Time
					programTime -= 60;
					if(programTime > elapsedTime)
					{
						menuPtr->settings->time = programTime;
					}
					else
					{
						event = EV_STOP;
					}
					break;
				case 2: // Intensity
					if(--menuPtr->settings->intensity < INTENSITY_LOW)
					{
						menuPtr->settings->intensity = INTENSITY_LOW;
					}
					event = EV_UPDATE_INTENSITY;
					break;
				case 3: // Infrared
					if(--menuPtr->settings->infrared < INTENSITY_LOW)
					{
						menuPtr->settings->infrared = INTENSITY_LOW;
					}
					event = EV_UPDATE_LED;
					break;
				default:
					break;
			} // currentRow
			break;
		// case KEY_LEFT:
		case KEY_RIGHT:
			switch(currentRow)
			{
				case 0: // Title
					event = EV_START;
					break;
				case 1: // Time
					programTime += 60;
					if((programTime) > SETTINGS_TMAX)
					{
						menuPtr->settings->time = SETTINGS_TMAX;
					}
					else
					{
						menuPtr->settings->time = programTime;
					}
					break;
				case 2: // Intensity
					if(++menuPtr->settings->intensity > INTENSITY_HIGH)
					{
						menuPtr->settings->intensity = INTENSITY_HIGH;
					}
					event = EV_UPDATE_INTENSITY;
					break;
				case 3: // Infrared
					if(++menuPtr->settings->infrared > INTENSITY_HIGH)
					{
						menuPtr->settings->infrared = INTENSITY_HIGH;
					}
					event = EV_UPDATE_LED;
					break;
				default: // other lines
					break;
			} // currentRow
			break;
		// case KEY_RIGHT:
		default:
			break;
	} // keyMask
	return event;
} // programMenuReactToKey()

controlEvent_t p4_1ReactToKey(uint8_t keyMask)
{
	controlEvent_t event = EV_NONE;
	programMenu_t *menuPtr = getMenuPtr(MENU_P4_1);
	int8_t currentRow = menuPtr->menu.selectedRow;
	int16_t programTime = menuPtr->settings->time;
	int16_t elapsedTime = menuPtr->settings->elapsedTime;
	switch(keyMask)
	{
		case KEY_DOWN:
			switch(currentRow)
			{
				case 3:
					// Set menu 4.2 at row 0
					(getMenuPtr(MENU_P4_2)->menu).selectedRow = 0;
					setMenu(MENU_P4_2);
					break;
				default:
					menuScroll(SCROLL_DOWN);
					break;
			}
			break;
		case KEY_UP:
			switch(currentRow)
			{
				case 0:
					// Set menu 4.2 at row 3
					(getMenuPtr(MENU_P4_2)->menu).selectedRow = 3;
					setMenu(MENU_P4_2);
					break;
				default:
					menuScroll(SCROLL_UP);
					break;
			}
			break;
		case KEY_LEFT:
			switch(currentRow)
			{
				case 0: //Title
					event = EV_STOP;
					setMenu(MENU_MAIN);
					break;
				case 2: // Time
					programTime -= 60;
					if(programTime > elapsedTime)
					{
						menuPtr->settings->time = programTime;
					}
					else
					{
						event = EV_STOP;
					}
					break;
				default:
					break;
			} // currentRow
			break;
		case KEY_RIGHT:
			switch(currentRow)
			{
				case 0: // Title
					event = EV_START;
					break;
				case 2: // Time
					programTime += 60;
					if((programTime) > SETTINGS_TMAX)
					{
						menuPtr->settings->time = SETTINGS_TMAX;
					}
					else
					{
						menuPtr->settings->time = programTime;
					}
					break;
				default:
					break;
			} //currentRow
			break;
		default:
			setMenu(MENU_MAIN);
			break;
	}
	return event;
}

controlEvent_t p4_2ReactToKey(uint8_t keyMask)
{
	controlEvent_t event = EV_NONE;
	programMenu_t *menuPtr = getMenuPtr(MENU_P4_2);
	int8_t currentRow = menuPtr->menu.selectedRow;
	switch(keyMask)
	{
		case KEY_DOWN:
			switch(currentRow)
			{
				case 3:
					// Set menu 4.1 at row 0
					(getMenuPtr(MENU_P4_1)->menu).selectedRow = 0;
					setMenu(MENU_P4_1);
					break;
				default:
					menuScroll(SCROLL_DOWN);
					break;
			}
			break;
		case KEY_UP:
			switch(currentRow)
			{
				case 0:
					// Set menu 4.1 at row 3
					(getMenuPtr(MENU_P4_1)->menu).selectedRow = 3;
					setMenu(MENU_P4_1);
					break;
				default:
					menuScroll(SCROLL_UP);
					break;
			}
			break;
		case KEY_LEFT:
			switch(currentRow)
			{
				case 0: // Intensity
					if(--menuPtr->settings->intensity < INTENSITY_LOW)
					{
						menuPtr->settings->intensity = INTENSITY_LOW;
					}
					event = EV_UPDATE_INTENSITY;
					break;
				case 1: // Infrared
					if(--menuPtr->settings->infrared < INTENSITY_LOW)
					{
						menuPtr->settings->infrared = INTENSITY_LOW;
					}
					event = EV_UPDATE_LED;
					break;
				case 2: // Frequency
					if(--(menuPtr->settings->frequency) < SETTINGS_FREQMIN)
					{
						menuPtr->settings->frequency = SETTINGS_FREQMIN;
					}
					event = EV_UPDATE_FREQUENCY;
					break;
				case 3: // Angle
					if((menuPtr->settings->angle -= 10) < SETTINGS_ANGMIN)
					{
						menuPtr->settings->angle = SETTINGS_ANGMIN;
					}
					event = EV_UPDATE_ANGLE;
					break;
			} // currentRow
			break;
			// KEY_LEFT
			case KEY_RIGHT:
				switch(currentRow)
				{
				case 0: // Intensity
					if(++menuPtr->settings->intensity > INTENSITY_HIGH)
					{
						menuPtr->settings->intensity = INTENSITY_HIGH;
					}
					event = EV_UPDATE_INTENSITY;
					break;
				case 1: // Infrared
					if(++menuPtr->settings->infrared > INTENSITY_HIGH)
					{
						menuPtr->settings->infrared = INTENSITY_HIGH;
					}
					event = EV_UPDATE_LED;
					break;
				case 2: // Frequency
					if(++(menuPtr->settings->frequency) > SETTINGS_FREQMAX)
					{
						menuPtr->settings->frequency = SETTINGS_FREQMAX;
					}
					event = EV_UPDATE_FREQUENCY;
					break;
				case 3: // Angle
					if((menuPtr->settings->angle += 10) > SETTINGS_ANGMAX)
					{
						menuPtr->settings->angle = SETTINGS_ANGMAX;
					}
					event = EV_UPDATE_ANGLE;
					break;
				} // currentRow
				break;
		default:
			setMenu(MENU_MAIN);
			break;
	}
	return event;
}

uint8_t checkKeyAccepted(uint8_t keyMask)
{
	static keyState_t keyState = ST_KEY_RELEASED;
	static uint8_t keyMaskPrevious = KEY_NONE;
	static uint8_t counter = 0;
	uint8_t accepted = 0;
	switch(keyState)
	{
		case ST_KEY_RELEASED:
			if(keyMask != KEY_NONE)
			{
				keyState = ST_KEY_PRESSED_ONCE;
				keyMaskPrevious = keyMask;
				accepted = 1;
			} // (keyMask != KEY_NONE)
			else
			{
				accepted = 0;
			}
			break;
		case ST_KEY_PRESSED_ONCE:
			accepted = 0;
			if(keyMask == keyMaskPrevious)
			{
				keyState = ST_KEY_COUNTING;
				counter = 0;
			} // (keyMask == keyMaskPrevious)
			else
			{
				keyState = ST_KEY_RELEASED;
				keyMaskPrevious = KEY_NONE;
			} // !(keyMask == keyMaskPrevious)
			break;
		case ST_KEY_COUNTING:
			accepted = 0;
			if(keyMask == keyMaskPrevious)
			{
				if(++counter == 5)
				{
					keyState = ST_KEY_PRESSED_MULTIPLE;
					counter = 0;
				}
			}
			else
			{
				keyMaskPrevious = KEY_NONE;
				keyState = ST_KEY_RELEASED;
				counter = 0;
			}
			break;
		case ST_KEY_PRESSED_MULTIPLE:
			if(keyMask == keyMaskPrevious)
			{
				accepted = 1;
			}// (keyMask == keyMaskPrevious)
			else
			{
				accepted = 0;
				keyState = ST_KEY_RELEASED;
				keyMaskPrevious = KEY_NONE;
			}// !((keyMask != KEY_NONE) && keyMask == keyMaskPrevious)
			break;
	} // keyState
	return accepted;
}

int8_t getNextFreq(programSettings_t* settings)
{
	int8_t nextFreq;

	// Get next frequency
	nextFreq = settings->freqArray[(settings->currentFreqIndex)];
	// Check if valid
	if(nextFreq > 0)
	{
		settings->currentFreqIndex++;
	}
	else
	{
		nextFreq = settings->freqArray[(settings->currentFreqIndex = 0)];
	}
	if(nextFreq < 1)
	{
		return settings->frequency;
	}
	return nextFreq;
}
