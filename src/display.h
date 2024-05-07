#ifndef TUERSPION_DISPLAY_H
#define TUERSPION_DISPLAY_H

#include "libraries.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Display Notifications
unsigned long notificationStartTime = 0;
unsigned long initialNotificationDuration = 3000;
unsigned long notificationDuration = 2000;
unsigned long lastNotificationTime = 0;

#endif
