/*
	Conifg.h - Wrangler for command line parameters that need to make it to internal
	objects that are not printers/boards.
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

#include "PrintVisualType.h"

class Config
{
	public:
		inline static Config& Get()
		{
			static Config c;
			return c;
		};

		// High-res extrusion parameter.
		inline void SetExtrusionMode(unsigned int iVal){ m_iExtrusion = iVal;}
		inline unsigned int GetExtrusionMode(){ return m_iExtrusion;}

		// Should extrusion be coloured by width?.
		inline void SetColourE(bool bVal){ m_bColorExtrusion = bVal;}
		inline bool GetColourE(){ return m_bColorExtrusion;}

		// Should extrusion be coloured by width?.
		inline void SetSkewCorrect(bool bVal){ m_bSkew = bVal;}
		inline bool GetSkewCorrect(){ return m_bSkew;}

		// Starting LCD colour scheme
		inline void SetLCDScheme(uint8_t iVal){ m_iScheme = iVal;}
		inline uint8_t GetLCDScheme(){ return m_iScheme;}

		// Use a "debug" core? (board-specific)
		inline void SetDebugCore(bool bVal){ m_bDebugCore = bVal;}
		inline bool GetDebugCore(){ return m_bDebugCore;}

	private:
		unsigned int m_iExtrusion = false;
		bool m_bColorExtrusion = false;
		bool m_bSkew = false;
		uint8_t m_iScheme = 0;
		bool m_bDebugCore = false;

};
