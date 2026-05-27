# LCDDisplay
Library for LCD Displays using I2C, PCF8574, MCP23008, etc. Also works for SPI and other I2C I/O Expanders by overriding two small methods.


## **PRELIMINARY**
_Content is being updated..._

## Blurb

Simple text-mode Liquid Crystal Displays have been around since the 1970s. The original displays needed many control lines (8 data bits, enable, read/write, chip select, etc).

These days, an I2C serial interface is usually used because it needs just two control lines (data SDA and clock SCL). SPI can also be used, but that needs 3 or 4 control lines (MOSI, MISO, SCK and CS).

To use I2C communications, an I2C I/O expander chip like the PCF8574 or MCP23008 is usually used. This converts I2C commands into parallel digital data to control the LCD.

There are many really cheap I2C adapter boards available which use the PCF8574 chip. These are specifically designed for the LCD1602 and LCD2004 displays (etc), but they will work with most displays. LCDs with the adapters already fitted are also common. The only problem with these is that they are designed for 5V, and most modern microcontrollers run on 3.3V. But there's an easy solution to that, see below.

Here's a "modern" retro LCD display with a blacklight 

![LCD Display](https://muman.ch/muman/lcd1602.png)

And here's a really old one from 1990, a Philips LTN221, using an I2C adapter with special wiring 

![Old LTN221 LCD Display](https://muman.ch/pub/ltn221.jpg)

## The Dangerous 3.3V and 5V Conundrum

The I2C I/O expander chip PCF8574 can run on 3.3V or 5V. But the LCD display itself runs on 5V, so you must use a 5V supply to the I2C module.

This can cause serious problems if you connect it to a 3.3V microcontroller which does not have 5V-tolerant inputs/outputs. The reason for this is that the PCF8574 boards have pullup resistors to 5V on the I2C communications pins SDA and SCL. The solution is to remove the 5V pullup resistors from the I2C module, and add external pullups to 3.3V. It's usually R8 and R9 which must be removed (do this at your own risk, see Disclaimer.) There are more details in the original article (https://muman.ch/muman/index.htm?muman-lcd-character-display.htm). 

![I2C Expander Schematic](https://muman.ch/muman/lcd-i2c-adapter-module-5.png)

![Pullup Resistors](https://muman.ch/muman/pcf8574-pullups.png)

Most microcontroller boards, Arduinos etc, already have on-board pullup resistors to 3.3V on the I2C's SDA and SCL lines, so you don't need to add them. (The Nucleo-64 boards that I use do NOT have the pullups.) Check the schematic for this. You may also have additional I2C modules connected to the bus which have their own pullup resistors. Ensure the total pullup resistance to 3.3V is between 1.5 and 6.8K ohms (resistances in parallel), and NEVER mix pullups to 3.3V and 5V. Up to 10K ohms will work too, but the higher the pullup resistance, the slower the max. speed of the bus. If I2C communications doesn't work, it's probably because of missing pullups.

## LCDDisplay Class Reference

Details of each method can be found in the source code's comments, or just by reading the very simple C++ code.

```cpp
class LCDDisplay
{
public:
		bool begin(TwoWire* twoWire, int slaveAddress, int rows, int columns);
		void backlightOn(bool backlightOn);
		void clearDisplay();
		void displayOn(bool displayOn);
		void cursorUnderline(bool show);
		void cursorBlink(bool show);
		void cursorHome();
		void cursorPos(int row, int column);
		void entryModeSet(ENTRYMODE mode);
		void putChar(char ch, int row, int column);
		void putChar(char ch);
		void putString(const char* s, int row, int column);
		void putString(const char* s);
		void writeCGRAM(int charIndex, const byte charData[8]);
		bool readCGRAM(int charIndex, byte charData[8]);
		bool readDDRAM(char* ch, int row, int column);
		bool readBusyFlagAndAddress(bool* bf, int* adds);
		bool isBusy();
		bool waitBusy();
		bool testDDRAM();
		bool testCGRAM();
};
```

The code is self-explanatory, except maybe for the `CGRAM` and `DDRAM` methods. `CGRAM` means Character Generator RAM, which holds data for 8 user-defined characters. See the LCD's data sheet for details of CGRAM programming.
`DDRAM` means Display Data RAM, which can be read to find out what's on the display, but that's not usually necessary.

## Example Sketch

```cpp
#include "LCDDisplay.h"
LCDDisplay lcd;

void setup()
{
	// start I2C master
	Wire.begin();
	Wire.setClock(400000);
	Wire.setTimeout(100);

	// initialize a 16x2 LCD on I2C address 0x27
	lcd.begin(&Wire, 0x27, 2, 16);
}

void loop()
{
	char buf[32];
	sprintf(buf, "Voltage: %dV  ", rand() % 100);
	lcd.putString(buf, 0, 0);
	sprintf(buf, "Current: %dmA ", rand() % 50);
	lcd.putString(buf, 1, 0);

	delay(500);
}

```

## I2C Address

The upper 4 bits of the card's 7-bit I2C address are fixed by the PCF8574 chip. The lower 3 bits are configured by the A2 A1 A0 jumpers on the expander board. The PCF8574 and PCF8574A have different internally hard-wired addresses.

|        | A6   | A5   | A4   | A3   | A2   | A1   | A0   |Addresses |
|:-------|:-----|:-----|:-----|:-----|:-----|:-----|:-----|:---------|
|PCF8574 |0     |1     |0     |0     |x     |x     |x     |0x20..0x27|
|PCF8574A|0     |1     |1     |1     |x     |x     |x     |0x38..0x3f|

If the jumper is in (closed), it is grounded and the address bit is 0. If the jumper is out (open), it is pulled high by an internal resistor and the bit is 1. For some boards, all jumpers are in (grounded, A2..A0 = 000), and a bit is set to 1 by cutting the trace. For others, all jumpers are out (pull-ups make A2..A0 = 111), and a bit is set to 0 with a blob of solder (which connects it to GND). 


## Using C++ inheritance to adapt the code for other I2C or SPI expanders

MCP23008 8-bit I2C Expander, or MCP23S08 SPI expander

TODO


## Missing Links

PCF8574 Data Sheet \
https://www.ti.com/lit/ds/symlink/pcf8574.pdf

MCP23008/MCP23S08 Data Sheet \
https://ww1.microchip.com/downloads/en/DeviceDoc/MCP23008-MCP23S08-Data-Sheet-20001919F.pdf

The original article, note that the code in the blog has been superceded by the LCDDisplay library \
https://muman.ch/muman/index.htm?muman-lcd-character-display.htm

Notes about I2C and SPI I/O Expanders \
https://muman.ch/muman/index.htm?muman-mcp23017.htm

The old Philips LTN221 LCD Data Sheet, for nostaliga purposes only \
https://muman.ch/pub/LTN221.pdf


## Revision History

| Date  | Revision | Description |
|:---------- |:---------|:----------- |
| 2026.xx.xx | 0.0.0	| Preliminary |

<br/>

## Joke of the Week

Matt's Tip \#32: _Never buy Placebos on Ebay. They are almost always fake!_



