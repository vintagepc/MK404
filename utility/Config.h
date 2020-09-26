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

class Config
{
	public:
		inline static Config& Get()
		{
			static Config c;
			return c;
		};

		// High-res extrusion parameter.
		inline void SetHRE(bool bVal){ m_bHiResExtrusion = bVal;}
		inline bool GetHRE(){ return m_bHiResExtrusion;}

	private:
		bool m_bHiResExtrusion = false;

};
