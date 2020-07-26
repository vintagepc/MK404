/*
	Scriptable.h - a base class for an object that is scriptable.

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

using namespace std;

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
		Scriptable(const string &strName="Unnamed"):IScriptable(strName){};

    protected:
		// Registers a new no-argument Scriptable action with the given function, description, and an ID that will be
		// provided in ProcessAction. This lets you set up an internal enum and switch() on actions
		// instead of needing to make a string-comparison if-else conditional.
		inline bool RegisterAction(const string &strAct, const string& strDesc, unsigned int ID) override
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
		inline bool RegisterActionAndMenu(const string &strAct, const string& strDesc, unsigned int ID)
		{
			if (RegisterAction(strAct, strDesc, ID))
			{
				RegisterMenu(strAct, ID);
				return true;
			}
			return false;
		}

		void RegisterMenu(const string &strLabel, uint uiID)
		{
			ScriptHost::AddMenuEntry(strLabel, uiID, this);
		}

		//Forwarder:
		inline void RegisterAction(const string &strAct, const string& strDesc, unsigned int ID, const vector<ArgType>& vTypes)
		{
			IScriptable::RegisterAction(strAct,strDesc,ID, vTypes);
		}
};
