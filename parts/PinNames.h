/*
	PinNames.h - Wrangler for convenience pin names (e.g. master list of all pins.)
	Not all boards have every pin, the idea is that if using TryConnect() you can potentially
	reuse a board and disconnect a pin in its wiring to reduce copypasta code.

	Copyright 2020-2021 VintagePC <https://github.com/vintagepc/>

 	This file is part of MK404.

	MK404 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MK404 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MK404.  If not, see <http://www.gnu.org/licenses/>.
 */


#pragma once

#define __PIN_COMBO(x,y) x##y

#define __TMC2130_PIN_SET(x) \
		__PIN_COMBO(x,_DIR_PIN), \
		__PIN_COMBO(x,_ENABLE_PIN),\
		__PIN_COMBO(x,_STEP_PIN), \
		__PIN_COMBO(x,_TMC2130_CS),\
		__PIN_COMBO(x,_TMC2130_DIAG),\

#define __A4982_PIN_SET(x) \
		__PIN_COMBO(x,_MS1_PIN), \
		__PIN_COMBO(x,_MS2_PIN), \
		__PIN_COMBO(x,_REF_PIN), \
		__PIN_COMBO(x,_RST_PIN), \
		__PIN_COMBO(x,_SLP_PIN), \

// Pin names. Just add yours here.
namespace PinNames {
	enum Pin{
		BEEPER,
		BTN_EN1,
		BTN_EN2,
		BTN_ENC,
		BTN_ARRAY,
		__TMC2130_PIN_SET(E0)
		__A4982_PIN_SET(E0)
		E0_FAN,
		E_MUX0_PIN,
		E_MUX1_PIN,
		FAN_PIN,
		FAN_1_PIN,
		FAN_2_PIN,
		FINDA_PIN,
		HEATER_0_PIN,
		HEATER_1_PIN,
		HEATER_2_PIN,
		HEATER_BED_PIN,
		__TMC2130_PIN_SET(I)
		IR_SENSOR_PIN,
		KILL_PIN,
		LCD_BL_PIN,
		LCD_PINS_D4,
		LCD_PINS_D5,
		LCD_PINS_D6,
		LCD_PINS_D7,
		LCD_PINS_ENABLE,
		LCD_PINS_RS,
		LED_PIN,
		MMU_HWRESET,
		__TMC2130_PIN_SET(P)
		PAT_INT_PIN,
		PS_ON_PIN,
		__TMC2130_PIN_SET(S)
		RX2_PIN,
		SDCARDDETECT,
		SDPOWER,
		SDSS,
		SHIFT_CLOCK,
		SHIFT_DATA,
		SHIFT_LATCH,
		SUICIDE_PIN,
		SWI2C_SCL,
		SWI2C_SDA,
		TACH_0,
		TACH_1,
		TACH_2,
		TEMP_0_PIN,
		TEMP_1_PIN,
		TEMP_2_PIN,
		TEMP_AMBIENT_PIN,
		TEMP_BED_PIN,
		TEMP_PINDA_PIN,
		TX2_PIN,
		UVLO_PIN,
		VOLT_BED_PIN,
		VOLT_IR_PIN,
		VOLT_PWR_PIN,
		W25X20CL_PIN_CS,
		X_MAX_PIN,
		X_MIN_PIN,
		__TMC2130_PIN_SET(X)
		__A4982_PIN_SET(X)
		Y_MAX_PIN,
		Y_MIN_PIN,
		__TMC2130_PIN_SET(Y)
		__A4982_PIN_SET(Y)
		Z_MAX_PIN,
		Z_MIN_PIN,
		__TMC2130_PIN_SET(Z)
		__A4982_PIN_SET(Z)
		MCP_CSEL,
		PIN_COUNT,
		INVALID_PIN // Used for testing error cases.

	};
}; // namespace PinNames
