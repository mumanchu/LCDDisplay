# LCDDisplay
Library for I2C LCDs using PCF8574, MCP23008 etc.

## **PRELIMINARY**
_Content is being updated..._

## Blurb
Simple LCD text-mode displays have been around since the 1970s. The original displays needed lots of control lines (8 data bits, enable, read/write, chip select, etc).
These days an I2C or SPI interface is usually used, which needs just two or four control lines (SDA and SCL for I2C, or MOSI, MISO, SCK and CS for SPI).

TODO

## Class Reference

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
		bool testDDRam();
		bool testCGRam();
};
```

## Using inheritance to adapt code for other I2C or SPI expanders

TODO


## Revision History
| Date  | Revision | Description |
|:---------- |:---------|:----------- |
| 2026.xx.xx | 0.0.0	| Preliminary |

<br/>

## Joke of the Week

Matt's Tip \#32: Never buy Placebos on Ebay. They are almost always fakes.



