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

#include "Printer.h"

#include "Scriptable.h"
#include <string>


Printer::Printer():Scriptable("Printer")
{
	RegisterAction("MouseBtn", "Simulates a mouse button (# = GL button enum, gl state)", ActMouseBtn, {ArgType::Int,ArgType::Int});
}

void Printer::SetVisualType(const std::string &visType) {
	m_visType = visType; OnVisualTypeSet(visType);
}

IScriptable::LineStatus Printer::ProcessAction(unsigned int iAct, const std::vector<std::string> &vArgs)
{
	switch (iAct)
	{
		case ActMouseBtn:
			OnMousePress(std::stoi(vArgs.at(0)),std::stoi(vArgs.at(1)),0,0);
			return LineStatus::Finished;
		default:
			return LineStatus::Unhandled;
	}
}
