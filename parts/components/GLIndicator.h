/*
	GLIndicator.h - Simple LED visualizer for IPC

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


#pragma once

#include "Color.h"
#include "Util.h"            // for hexColor_t
#include <atomic>
#include <cstdint>          // for uint32_t, uint8_t

class GLIndicator
{
public:
	// Creates a new LED with RGBA color (A ignored) uiHexColor and char label chrLabel
	explicit GLIndicator(char chrLabel, bool bInvert = false, bool bBlackBG = false);
	// Creates a version that uses colour interp
	GLIndicator(bool bInterpMode, char chrLabel);
	// Draws the LED
	void Draw();

	void SetValue(uint8_t value);

	inline void SetLabel(char label) { m_chrLabel = label;}
	inline char GetLabel() { return m_chrLabel.load();}

	inline void SetColor(uint32_t color) { m_Color = hexColor_t(color); }

	inline void SetColor(hexColor_t color) { m_Color = color; }

	inline uint16_t RotateStep(uint16_t uiAngle) { return (m_uiRot = (uiAngle + m_uiRot) % 360U); }

	inline void SetVisible(bool bVisible) { m_bVisible = bVisible; }

	inline void SetDisabled(bool bDisabled) { m_bDisabled = bDisabled; }

	// Expects valid range 0-255, values outside this range are clamped by the lerp code internally.
	inline void SetLerp(int16_t uiVal) { m_uiLerpVal = uiVal; }

private:
	// Value changed callback.
	hexColor_t m_Color = hexColor_t(0x00FF0000);
	std::atomic_char m_chrLabel {' '};
	std::atomic_uint8_t m_uiBrightness = {0};
	bool m_bInvert = false;
	std::atomic_uint16_t m_uiRot {0};
	bool m_bVisible = false;
	bool m_bBlackBG = false;
	bool m_bInterp = false;
	std::atomic_bool m_bDisabled {0};
	std::atomic_int16_t m_uiLerpVal = {0};

	static constexpr Color3fv m_colColdTemp = {0, 1, 1};
	static constexpr Color3fv m_colHotTemp = {1, 0, 0};
};
