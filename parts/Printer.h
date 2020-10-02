/*
	Printer.h - Printer interface for printer assemblies.
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

#include "GLHelper.h"
#include "IScriptable.h"
#include "Scriptable.h"
#include <string>
#include <utility>
#include <vector>

class Printer: public Scriptable
{
	public:
		enum class VisualType
		{
			MINIMAL = 0x0,
			SIMPLE = 0x1,
			ADVANCED = 0x2,
		};

		Printer();

		// GL methods, use these to render your printer visuals and
		virtual void Draw(){}; // pragma: LCOV_EXCL_START

		// Passthrough for GlutMouseFunc
		virtual void OnMousePress(int /*button*/, int /*action*/, int /*x*/, int /*y*/){};

		// Passthrough for GlutMotionFunc
		virtual void OnMouseMove(int /*y*/, int /*y*/){};

		// Overload this if you need to setup your visuals.
		virtual void OnVisualTypeSet(const std::string &/*type*/){};

		virtual std::pair<int,int> GetWindowSize() = 0; // pragma: LCOV_EXCL_STOP

		inline std::string GetVisualType() { return m_visType; }

		void SetVisualType(const std::string &visType);

		inline void SetConnectSerial(bool bVal){m_bConnectSerial = bVal;}

	protected:
		inline bool GetConnectSerial(){return m_bConnectSerial;}

		LineStatus ProcessAction(unsigned int iAct, const std::vector<std::string> &vArgs) override;

		GLHelper m_gl{};

	private:
		std::string m_visType = "lite";
		bool m_bConnectSerial = false;

		std::string m_strPrintName;

		enum Actions
		{
			ActKeyPress,
			ActMouseBtn
		};

};
