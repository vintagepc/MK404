/*
	Einsy.cpp - MK404 printer simulator for the Prusa i3 MK2/3 range.
	Dubbed MK404 as a tribute to their web 404 page. ;-)

	Copyright 2020 VintagePC <https://github.com/vintagepc/>

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

#define CATCH_CONFIG_MAIN
#include "3rdParty/catch2/catch.hpp"
#include "ADC_Buttons.h"
#include "Beeper.h"
#include "Board.h"
#include "EEPROM.h"
#include "Fan.h"
#include "GLHelper.h"
#include "Heater.h"
#include "HD44780.h"
#include "IRSensor.h"
#include "IScriptable.h"
#include "PAT9125.h"
#include "PINDA.h"
#include "SDCard.h"
#include "SerialLineMonitor.h"
#include "Test_Board.h"
#include "Thermistor.h"
#include "TMC2130.h"
#include "VoltageSrc.h"
#include "w25x20cl.h"

#ifndef TEST_MODE
	#error "Internal_Tests requires TEST_MODE defined to access protected interface functions."
#endif

static constexpr unsigned int ACT_MAX = -1;

#define _INTERNAL_CASE(x,y) TEST_CASE(#x#y)

#define INTERNAL_CASE(name) _INTERNAL_CASE(Internal_Unhandled_,name)

using LineStatus = IScriptable::LineStatus;

#define UNHANDLED_TEST(object) INTERNAL_CASE(object) \
	{ \
		object o; \
		REQUIRE(o.Test_ProcessActionIF(ACT_MAX,{}) == LineStatus::Unhandled); \
	}

#define UNHANDLED_TEST_A(object, ...) INTERNAL_CASE(object) \
	{ \
		object o(__VA_ARGS__); \
		REQUIRE(o.Test_ProcessActionIF(ACT_MAX,{}) == LineStatus::Unhandled); \
	}


UNHANDLED_TEST_A(ADC_Buttons, "");
UNHANDLED_TEST(Beeper);
UNHANDLED_TEST(Boards::Test_Board);
UNHANDLED_TEST(EEPROM);
UNHANDLED_TEST_A(Fan,1000,'F');
UNHANDLED_TEST(GLHelper);
UNHANDLED_TEST(HD44780);
UNHANDLED_TEST_A(Heater, 1.F, 25.F, false, 'H', 30.F, 35.F)
UNHANDLED_TEST(IRSensor);
UNHANDLED_TEST(PAT9125);
UNHANDLED_TEST(PINDA);
UNHANDLED_TEST(SDCard);
UNHANDLED_TEST_A(SerialLineMonitor,"SLM");
UNHANDLED_TEST(Thermistor);
UNHANDLED_TEST_A(TMC2130, 'X');
UNHANDLED_TEST(VoltageSrc);
UNHANDLED_TEST(w25x20cl);

void Test_IRSensor_OOR() {
	IRSensor o;
	REQUIRE(o.Test_ProcessActionIF(IRSensor::ActSet, {std::to_string(IRSensor::IRState::IR_MIN)}) == LineStatus::Error);
	REQUIRE(o.Test_ProcessActionIF(IRSensor::ActSet, {std::to_string(IRSensor::IRState::IR_MIN -1)}) == LineStatus::Error);
	REQUIRE(o.Test_ProcessActionIF(IRSensor::ActSet, {std::to_string(IRSensor::IRState::IR_MAX +1)}) == LineStatus::Error);
	REQUIRE(o.Test_ProcessActionIF(IRSensor::ActSet, {std::to_string(IRSensor::IRState::IR_MAX)}) == LineStatus::Error);
};

// Out of range tests
TEST_CASE("Internal_IRSensor_OOR") {
	Test_IRSensor_OOR();
}

void Boards::Test_Board_Interface() {
	Boards::Test_Board b;
	REQUIRE(b.TryConnect(nullptr, Pin::INVALID_PIN) == false);
	REQUIRE(b.TryConnect(Pin::INVALID_PIN, nullptr, 0) == false);
	REQUIRE(b.TryConnect(nullptr, 0, Pin::INVALID_PIN) == false);
	REQUIRE(b.GetPWMIRQ(Pin::INVALID_PIN) == nullptr);
	REQUIRE(b.GetDIRQ(Pin::INVALID_PIN) == nullptr);
};

TEST_CASE("Internal_Board_Connect_Errors") {
	Boards::Test_Board_Interface();
}

void Test_HD44780_OOR() {
	HD44780 d;
	REQUIRE(d.Test_ProcessActionIF(HD44780::ActCheckCGRAM, {"A","64"}) == LineStatus::Error);
	REQUIRE(d.Test_ProcessActionIF(HD44780::ActCheckCGRAM, {"128","60"}) == LineStatus::Timeout);
	// Desync also indicates all lines have changed.
	REQUIRE(d.Test_ProcessActionIF(HD44780::ActDesync,{}) == LineStatus::Finished);
	// Line OOR
	REQUIRE(d.Test_ProcessActionIF(HD44780::ActWaitForText, {"A","10"}) == LineStatus::Error);
	REQUIRE(d.Test_ProcessActionIF(HD44780::ActWaitForText, {"A","-2"}) == LineStatus::Error);
};

// Out of range tests
TEST_CASE("Internal_HD44780_OOR") {
	Test_HD44780_OOR();
}

void Test_PAT9125_Toggle() {
	PAT9125 p;
	p.Set(PAT9125::FS_MIN);
	p.Toggle();
	REQUIRE(p.m_state == PAT9125::FS_MIN);
	// Check that writes are blocked to RO:
	auto uiOld = p.GetRegVal(0);
	REQUIRE(p.SetRegVal(0,~uiOld) == false);
	REQUIRE(p.GetRegVal(0) == uiOld);

	REQUIRE(p.Test_ProcessActionIF(PAT9125::ActSet, {std::to_string(PAT9125::FS_MAX)}) == LineStatus::Error);
	REQUIRE(p.Test_ProcessActionIF(PAT9125::ActSet, {std::to_string(PAT9125::FS_MIN)}) == LineStatus::Error);

};

// Out of range tests
TEST_CASE("Internal_PAT9125") {
	Test_PAT9125_Toggle();
}

void Test_PINDA_OOR() {
	PINDA p;
	REQUIRE(p.Test_ProcessActionIF(PINDA::ActSetMBLPoint, {"-1","1.0"}) == LineStatus::Error);
	REQUIRE(p.Test_ProcessActionIF(PINDA::ActSetMBLPoint, {"49","1.0"}) == LineStatus::Error);
	REQUIRE(p.Test_ProcessActionIF(PINDA::ActSetXYCalPont, {"-1","1.0","1.0"}) == LineStatus::Error);
	REQUIRE(p.Test_ProcessActionIF(PINDA::ActSetXYCalPont, {"4","1.0","1.0"}) == LineStatus::Error);
};

// Out of range tests
TEST_CASE("Internal_PINDA_OOR") {
	Test_PINDA_OOR();
}


void Test_w25x20cl_errors() {
	w25x20cl w;
	// Check behaviour for no filename given.
	REQUIRE(w.Test_ProcessActionIF(w25x20cl::ActLoad, {}) == LineStatus::Error);
	REQUIRE(w.Test_ProcessActionIF(w25x20cl::ActSave, {}) == LineStatus::Error);
};

TEST_CASE("Internal_w25x20cl_errors") {
	Test_w25x20cl_errors();
}
