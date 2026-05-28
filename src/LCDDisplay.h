#pragma once

/////////////////////////////////////////////////////////////////////
// Library for LCD Displays using I2C, for PCF8574 GPIO Expander 
// Copyright (C) 2026.05.27, mumanchu + muman.ch
// All rights reversed
// 
// See github for details
// https://github/mumanchu/LCDDisplay
// https://muman.ch/muman/index.htm?muman-lcd-character-display.htm

#include <Wire.h>

class LCDDisplay
{
protected:
	TwoWire* wire;

	// Control bits, usually bits 0..3, see I2C adapter circuit diagram
	// https://muman.ch/muman/lcd-i2c-adapter-module-5.png
	enum CTRL
	{
		NONE = 0,
		RS = 0x01,      // P0 RS  Register select: 0=Instruction register (for write) or
		                //        Busy register(for read); 1=Data register(for read and write)
		RW = 0x02,      // P1 RW  Read/Write: 0=Write mode; 1=Read mode
		EN = 0x04,      // P2 EN  Enable
		BL = 0x08       // P3 BL  Backlight: 1=on; 0=off (also controlled by jumper)
	};

	// Saved values for Display on/off control command, D C B bits
	// all three bits are written by the command, so we must keep track of the current values
	enum DISP
	{
		DISPON = 0x04,  // D display on/off
		CURSOR = 0x02,  // C cursor on/off
		BLINK = 0x01    // B cursor blink on/off
	};

	int slaveAddress;   // I2C slave address
	int rows;           // 1, 2, 4
	int columns;        // 8, 16, 20, 40
	CTRL backLight;     // bit P3 is the backlight, this holds the P3 value
	DISP dispCtrl;      // current value of LCD's display control register
	int* rowOffset;     // address offsets to the start of each LCD line (DDRAM memory addresses)

public:
	enum ENTRYMODE
	{
		SH = 0x01,      // display shift: 0=not shifted; 1=display shifted (ID 1=shift left, ID 0=shift right)
		ID = 0x02       // cursor movement (DDRAM and CGRAM address): 1=increment; 0=decrement
	};

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

protected:
	bool writeCommand(int data);
	bool writeData(int data);
	bool writeByte(CTRL ctrl, int data);
	bool writeNibble(CTRL ctrl, int data);
	bool readByte(CTRL ctrl, byte* b);

	// override these in a derived class if using a different I2C I/O expander
	// as in MCP23008Lcd.h
	virtual bool readNibble(byte* nibble, byte ctrl);
	virtual bool write(byte b);
};



// Initialise Wire before calling this, see example sketch
bool LCDDisplay::begin(TwoWire* twoWire, int slaveAddress, int rows, int columns)
{
	wire = twoWire;

	if (rows != 1 && rows != 2 && rows != 4)
		return false;
	switch (columns) {
	case 8:
	case 12:
	case 16:
	case 20:
	case 24:
	case 40:
		break;
	default:
		return false;
	}
	this->slaveAddress = slaveAddress;
	this->rows = rows;
	this->columns = columns;

	// for 4-line displays, the offset for the last 2 lines depends on characters-per-line
	rowOffset = new int[4] { 0x00, 0x40, columns, 0x40 + columns };

	// the lcd starts in 8-bit mode
	// send several "set 4-bit mode" commands (0x03) with mysterious delays in between
	// see LCD documentation
	// function set: RS=0; RW=0; 4-bit data=0011
	writeNibble(CTRL::NONE, 0);
	delay(40);
	writeNibble(CTRL::NONE, 0x03);
	delay(5);
	writeNibble(CTRL::NONE, 0x03);
	delay(5);
	writeNibble(CTRL::NONE, 0x03);
	delay(5);

	// set 4-bit mode
	writeNibble(CTRL::NONE, 0x02);
	delay(1);

	// Function Set
	// configure the LCD for number of lines and font size
	// this cannot be changed later
	// command = 001dnf00
	// d : 1=8-bit, 0=4-bit data
	// n : 1=2 lines, 0=1 lines
	// f : font size 1=5x11, 0=5x8
	// for example:
	// n f   lines   size
	// 0 0   1       5x8
	// 0 1   1       5x11
	// 1 x   2       5x8    <- we use this one
	if (!writeCommand(0x28))
		return false;					// lcd not present or not responding

	// Entry Mode Set
	// addresses increment, cursor moves right
	entryModeSet(ENTRYMODE::ID);

	// set defaults, LCD has no hardware reset
	backlightOn(true);
	displayOn(true);
	clearDisplay();
	cursorHome();
	cursorBlink(false);
	cursorUnderline(false);

	return true;
}

// Note: With some displays you can't see anything when the backlight is off!
// this could be used to flash the display
// the backlight on my module draws 10mA
// not all displays have a backlight
void LCDDisplay::backlightOn(bool backlightOn)
{
	// save the bit state, it's written on every command (PCF8574 output P3)
	backLight = backlightOn ? CTRL::BL : CTRL::NONE;

	// just set/clear the bit
	write((byte)backLight);
}

// Clears the display by filling it with spaces
// the cursor position remains unchanged
void LCDDisplay::clearDisplay()
{
	writeCommand(0x01);

	// command takes 1.53ms or up to 3mS on old LCDs
	waitBusy();
}

// All characters are hidden when the display is off, but nothing else is changed
// this can be used to flash the text on the display
void LCDDisplay::displayOn(bool displayOn)
{
	int ctrl = dispCtrl;
	if (displayOn)
		ctrl |= DISP::DISPON;
	else
		ctrl &= ~DISP::DISPON;

	// save the bit state, it's written on every command
	dispCtrl = (DISP)ctrl;

	writeCommand(0x08 | ctrl);
}

// Show or hide the static underline cursor '_'
void LCDDisplay::cursorUnderline(bool show)
{
	int ctrl = dispCtrl;
	if (show)
		ctrl |= DISP::CURSOR;
	else
		ctrl &= ~DISP::CURSOR;
	dispCtrl = (DISP)ctrl;

	writeCommand(0x08 | ctrl);
}

// Show or hide the blinking box cursor
void LCDDisplay::cursorBlink(bool show)
{
	int ctrl = (int)dispCtrl;
	if (show)
		ctrl |= DISP::BLINK;
	else
		ctrl &= ~DISP::BLINK;
	dispCtrl = (DISP)ctrl;

	writeCommand(0x08 | ctrl);
}

// Move cursor to 0, 0
void LCDDisplay::cursorHome()
{
	writeCommand(0x02);

	// command takes 1.53ms
	waitBusy();
}

void LCDDisplay::cursorPos(int row, int column)
{
	writeCommand(0x80 | (rowOffset[row] + column));
}

// Entry Mode Set, sets display shift and cursor movement direction 
// ENTRYMODE::SH : Bit 0 : Display shift: 0=not shifted; 1=display shifted
//                 SH=1: ID=1 shift left, ID=0 shift right
// ENTRYMODE::ID : Bit 1 : Cursor movement (DDRAM and CGRAM address):
//                 1=increment; 0=decrement
// Note: "Shift mode" means that BOTH lines are shifted, which is probably
// not what you want! I can't think of any use for this strange feature.
// You can do something far better just by re-writing the entire line with
// PutString() or PutLines().
void LCDDisplay::entryModeSet(ENTRYMODE mode)
{
	// command = 000001is
	writeCommand(0x04 | (int)mode);
}

// Writes a single character to the given cursor location
// and moves the cursor to the next position
void LCDDisplay::putChar(char ch, int row, int column)
{
	cursorPos(row, column);
	writeData(ch);
}

// Writes a string at the given location
// and moves the cursor the next position
void LCDDisplay::putString(const char* s, int row, int column)
{
	cursorPos(row, column);
	putString(s);
}

// Writes a single character to the current cursor location
// and moves the cursor to the next position
void LCDDisplay::putChar(char ch)
{
	writeData(ch);
}

// Writes a string at the current cursor location
// and moves the cursor to the next position
void LCDDisplay::putString(const char* s)
{
	for (int i = 0; s[i] != 0; i++)
		writeData(s[i]);
}

// Write programmable character 0..7 in "character generater RAM" (CGRAM)
// character is assumed to be 5x8: 5-bit data width (0x00..0x1f) x 8 rows
// NOTE: After calling this, you must set the cursor position (DDRAM address)
// charIndex = 0..7
void LCDDisplay::writeCGRAM(int charIndex, const byte charData[8])
{
	// set CGRAM address
	writeCommand(0x40 | (charIndex * 8));

	// write the bitmap, 8 x 5-bit values
	for (int i = 0; i < 8; ++i) {
		writeData(charData[i] & 0x1f);
	}
}

bool LCDDisplay::readDDRAM(char* ch, int row, int column)
{
	writeCommand(0x80 | (rowOffset[row] + column));
	return readByte(CTRL::RS, (byte*)ch);
}

bool LCDDisplay::readCGRAM(int charIndex, byte charData[8])
{
	writeCommand(0x40 | (charIndex * 8));

	for (int i = 0; i < 8; ++i) {
		if (!readByte(CTRL::RS, charData + i))
			return false;
	}
	return true;
}

bool LCDDisplay::readBusyFlagAndAddress(bool* bf, int* adds)
{
	byte b;
	if (!readByte(CTRL::NONE, &b))
		return false;
	*bf = b & 0x80 ? 1 : 0;
	*adds = b & 0x7f;
	return true;
}

bool LCDDisplay::isBusy()
{
	bool bf;
	int adds;
	if (readBusyFlagAndAddress(&bf, &adds))
		return bf;
	return false;
}

// Waits for up to 10mS for busy flag BF to go false
// returns false if it timed out
bool LCDDisplay::waitBusy()
{
	unsigned long t = micros();
	while (isBusy()) {
		yield();
		unsigned long elapsed = micros() - t;
		if (elapsed > 10000) {
			LOGERROR("waitBusy timeout");
			return false;
		}
	}
	return true;
}


// Internal Methods
// "There is madness in his methods" - Liamwil Peareshakes

// Write a command (instruction), RS=0, RW=0
// data = 8-bits, sent as two nibbles, ms-nibble first
inline bool LCDDisplay::writeCommand(int data)
{
	return writeByte(CTRL::NONE, data);
}

inline bool LCDDisplay::writeData(int data)
{
	return writeByte(CTRL::RS, data);
}

inline bool LCDDisplay::writeByte(CTRL ctrl, int data)
{
	return writeNibble(ctrl, data >> 4) && writeNibble(ctrl, data);
}

// Writes 4 bits of data and the control bits (BL RW RS),
// toggles the EN bit and handles the timing.
// The LCD runs in 4-bit mode. To write a byte, this method is called
// twice, first for the  MS nibble and again for the LS nibble.
bool LCDDisplay::writeNibble(CTRL ctrl, int data)
{
	// get data in bits 7..4
	data = (data << 4) & 0xf0;

	// merge in the backlight and ctrl values
	int txbyte = (int)backLight | (int)ctrl | data;

	// set up the data, EN=0
	if (!write((byte)txbyte))
		return false;

	// set EN
	if (!write((byte)(txbyte | (int)CTRL::EN)))
		return false;

	// clear EN
	if (!write((byte)txbyte))
		return false;

	return true;
}

bool LCDDisplay::readByte(CTRL ctrl, byte* b)
{
	byte n1, n2;
	if (!readNibble(&n1, ctrl) || !readNibble(&n2, ctrl))
		return false;
	*b = (n1 << 4) + n2;
	return true;
}


// Override these in a derived class if using a different I2C I/O expander
// as in LcdLTN221.h

bool LCDDisplay::readNibble(byte* nibble, byte ctrl)
{
	// merge in the backlight and ctrl values
	// RW   = read
	// ctrl = NONE or RS
	byte txbyte = backLight | ctrl | CTRL::RW;

	// set up the data, EN=0
	// bits 7..4 := 1 so they work as "pseudo inputs"
	if (!write(txbyte | 0xf0))
		return false;

	// set EN
	if (!write(txbyte | 0xf0 | CTRL::EN))
		return false;

	// read nibble in bits 7..4
	byte b;
	wire->requestFrom(slaveAddress, 1);
	if (wire->readBytes(&b, 1) != 1) {
		LOGERROR("readBytes failed");
		return false;
	}

	// clear EN
	if (!write(txbyte))
		return false;

	*nibble = (b >> 4);
	return true;
}

bool LCDDisplay::write(byte b)
{
	wire->beginTransmission(slaveAddress);
	wire->write(b);
	if (wire->endTransmission() != 0) {
		LOGERROR("write failed");
		return false;
	}
	return true;
}


// Read-after-write tests of the display's memory
bool LCDDisplay::testDDRAM()
{
	bool ok = true;
	clearDisplay();
	byte data[rows][columns];
	for (int row = 0; row < rows; ++row) {
		for (int col = 0; col < columns; ++col) {
			byte ch = (byte)rand();
			data[row][col] = ch;
			putChar(ch, row, col);
		}
	}
	cursorHome();
	for (int row = 0; row < rows; ++row) {
		for (int col = 0; col < columns; ++col) {
			char ch;
			readDDRAM(&ch, row, col);
			if (data[row][col] != ch) {
				ok = false;
				LOGERROR("dd ram compare error");
			}
		}
	}
	clearDisplay();
	return ok;
}

bool LCDDisplay::testCGRAM()
{
	byte cg[8][8];
	bool ok = true;

	for (int i = 0; i < 8; ++i) {
		for (int j = 0; j < 8; ++j) {
			cg[i][j] = rand() & 0x1f;
		}
		writeCGRAM(i, cg[i]);
	}
	for (int i = 0; i < 8; ++i) {
		byte cg1[8];
		readCGRAM(i, cg1);

		for (int j = 0; j < 8; ++j) {
			if (cg[i][j] != cg1[j]) {
				ok = false;
				LOGERROR("cg ram compare error");
			}
		}
	}
	return ok;
}


