/////////////////////////////////////////////////////////////////////
// LCDDisplay library example sketch
// https://github.com/mumanchu/LCDDisplay
// Original article
// https://muman.ch/muman/index.htm?muman-lcd-character-display.htm

// I2C
#include <Wire.h>

// Normally #include "MumanchuDebug.h" for these definitions
#if !defined(LOGERROR)
#define LOGERROR(s) { Serial.println(s); Serial.flush(); }
#endif

#include "LCDDisplay.h"
LCDDisplay lcd;


void setup()
{
	Serial.begin(115200);
	delay(3000);
	Serial.println("\n\rStarted...\n\r");
	Serial.flush();

	pinMode(LED_BUILTIN, OUTPUT);

	// start the I2C master at 400KHz
	// max. speed depends on your hardware
	Wire.begin();
	Wire.setClock(400000);
	Wire.setTimeout(100);

	// initialize a 2 lines x 16 character LCD on I2C address 0x27
	lcd.begin(&Wire, 0x27, 2, 16);

	// test the LCD's RAM (just for fun)
	if (!lcd.testDDRAM()) {
		Serial.println("testDDRAM() failed");
		Serial.flush();
		while (1) yield();
	}
	if (!lcd.testCGRAM()) {
		Serial.println("testCGRAM() failed");
		Serial.flush();
		while (1) yield();
	}
}

void loop()
{
	// scheduler
	ulong t = millis();

	// every 500ms
	static ulong t1 = 0;
	if (t - t1 >= 500) {
		t1 = t;

		// toggle the led so we know it's running
		digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

		// format some random data
		char buf1[32];
		char buf2[32];
		sprintf(buf1, "Voltage: %2dV  ", rand() % 100);
		sprintf(buf2, "Current: %2dmA ", rand() % 50);

		// and display it
		lcd.putString(buf1, 0, 0);
		lcd.putString(buf2, 1, 0);
	}
 }
