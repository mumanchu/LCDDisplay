#pragma once

/////////////////////////////////////////////////////////////////////
// LCD Display via an MCP23008 8-bit I2C Expander
// Copybright (C) muman.ch, 2025
// All rights reversed
/*
This example uses inheritance to get most of its behaviour from 
the LCDDisplay class. It also uses the MCP23Expander library.
*/

#pragma once

#include "LCDDisplay.h"

#define MCP23EXPANDER8BIT_I2C
#include "MCP23Expander8bit.h"


class MCP23008Lcd : public LCDDisplay
{
protected:
	MCP23Expander8bitI2C mcp23008;

public:
	bool begin(TwoWire* twoWire, int i2cAddress, int rows, int columns);

protected:
	bool readNibble(byte* nibble, byte ctrl);
	bool write(byte b);
};

// Initialize Wire before calling this
bool MCP23008Lcd::begin(TwoWire* twoWire, int i2cAddress, int rows, int columns)
{
	mcp23008.begin(twoWire, i2cAddress);

	// configure all gpios as outputs
	mcp23008.configureGpios(0x00, 0x00, 0x00);

	// call the base class begin()
	return LCDDisplay::begin(twoWire, i2cAddress, 2, 20);
}

// Reads 4 bits from D7..D4 of the MCP23008 I2C I/O expander
bool MCP23008Lcd::readNibble(byte* nibble, byte ctrl)
{
	// merge in the backlight and ctrl values
	byte txbyte = backLight | ctrl | CTRL::RW;

	// configure D7..D4 gpios as inputs
	mcp23008.configureGpios(0xf0, 0, 0);

	// set up the data, EN=0
	if (!write(txbyte))
		return false;

	// set EN
	if (!write(txbyte | CTRL::EN))
		return false;

	byte b;
	if (!mcp23008.readGpios(&b))
		return false;

	// clear EN
	if (!write(txbyte))
		return false;

	// reconfigure gpios as outputs
	mcp23008.configureGpios(0, 0, 0);

	*nibble = (b >> 4);
	return true;
}

// Writes a byte to the outputs of the MCP23008 I2C I/O expander
bool MCP23008Lcd::write(byte b)
{
	return mcp23008.writeOutputs(b);
}

