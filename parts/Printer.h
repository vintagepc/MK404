/*
	Printer.h - Printer interface for printer assemblies.
	Copyright 2020 VintagePC <https://github.com/vintagepc/>

 	This file is part of MK3SIM.

	MK3SIM is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MK3SIM is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MK3SIM.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <stdlib.h>
#include <utility>

class Printer
{
	public:
		enum class VisualType
		{
			MINIMAL = 0x0,
			SIMPLE = 0x1,
			ADVANCED = 0x2,
		};

		Printer(){};
		// GL methods, use these to render your printer visuals and
		virtual void Draw(){};

		// Passthrough for GlutKeyboardFunc
		virtual void OnKeyPress(unsigned char key, int x, int y){};

		// Passthrough for GlutMouseFunc
		virtual void OnMousePress(int button, int action, int x, int y){};

		// Passthrough for GlutMotionFunc
		virtual void OnMouseMove(int x, int y){};

		// Overload this if you need to setup your visuals.
		virtual void OnVisualTypeSet(VisualType type){};

		virtual std::pair<int,int> GetWindowSize() = 0;

		VisualType GetVisualType() { return m_visType; }
		void SetVisualType(VisualType visType) {m_visType = visType; OnVisualTypeSet(visType);}

		inline void SetConnectSerial(bool bVal){m_bConnectSerial = bVal;}

	protected:
		bool GetConnectSerial(){return m_bConnectSerial;}

	private:
		VisualType m_visType = VisualType::MINIMAL;
		bool m_bConnectSerial = false;

};
