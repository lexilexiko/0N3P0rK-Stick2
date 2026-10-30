// M5StickC Plus 2 is a plain ESP32. The USB plug is a CH9102 on UART0,
// so the console is Serial. Serial1/Serial2 are extra UARTs and are not
// the USB log. Do not rename Serial to Serial2 (that is the Cardputer USB path).
#pragma once
#if CONFIG_IDF_TARGET_ESP32S2
#include <Arduino.h>
#include <USBCDC.h>
#ifndef Serial
extern USBCDC USBSerial;
#define Serial USBSerial
#endif
#endif
