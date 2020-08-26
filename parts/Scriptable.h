/*
	Scriptable.h - a base class for an object that is scriptable.

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

#include <stdlib.h>
#include <string>
#include <map>
#include <vector>
#include "ScriptHost.h"
#include "IScriptable.h"

class Scriptable: public IScriptable
{
	friend ScriptHost;
    public:
		Scriptable(const std::string &strName="Unnamed"):IScriptable(strName){};

    protected:
		// Registers a new no-argument Scriptable action with the given function, description, and an ID that will be
		// provided in ProcessAction. This lets you set up an internal enum and switch() on actions
		// instead of needing to make a string-comparison if-else conditional.
		inline bool RegisterAction(const std::string &strAct, const std::string& strDesc, unsigned int ID) final
		{
			if (IScriptable::RegisterAction(strAct,strDesc,ID))
			{
				ScriptHost::AddScriptable(m_strName,this);
				m_bRegistered = true;
				return true;
			}
			else
				return false;
		}

		// Convenience wrapper that also adds the action as a context menu entry.
		inline bool RegisterActionAndMenu(const std::string &strAct, const std::string& strDesc, unsigned int ID)
		{
			if (RegisterAction(strAct, strDesc, ID))
			{
				RegisterMenu(strAct, ID);
				return true;
			}
			return false;
		}

		void RegisterMenu(const std::string &strLabel, unsigned uiID)
		{
			ScriptHost::AddMenuEntry(strLabel, uiID, this);
		}

		//Forwarder:
		inline void RegisterAction(const std::string &strAct, const std::string& strDesc, unsigned int ID, const std::vector<ArgType>& vTypes)
		{
			IScriptable::RegisterAction(strAct,strDesc,ID, vTypes);
		}
};
