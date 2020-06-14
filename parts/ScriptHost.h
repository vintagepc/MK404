/*
	ScriptHost.h - Core handler responsible for handling
	scripting actions.

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
#include <memory>
#include <map>
#include <vector>
#include <sim_avr.h>

// Forward declare.
class Scriptable;

class ScriptHost
{
	//friend ScriptHost;
    public:
		static shared_ptr<ScriptHost> Get()
		{
			return g_pHost;
		}

		inline static bool IsInitialized()
		{
			return g_pHost!=nullptr;
		}
		static shared_ptr<ScriptHost> Init(std::string strFile, unsigned int uiFreq)
		{
			if (g_pHost!=nullptr)
			{
				fprintf(stderr,"ERROR: Duplicate initialization attempt for scripthost!\n");
				return g_pHost;
			}
			new ScriptHost(strFile,uiFreq);
			return g_pHost;
		}

		static void AddScriptable(string strName, Scriptable* src);

		static inline bool IsRegistered(string strName)
		{
			return m_clients.count(strName)!=0;
		}

		static void PrintScriptHelp();

		static void OnAVRCycle();

    private:
		static void LoadScript(const string &strScript);
		static void ParseLine(unsigned int iLine);
		static bool GetLineParts(const string &strLine, string &strCtxt, string& strAct, vector<string>&vArgs);

		//We can't register ourselves as a scriptable so just fake it with a processing func.
		static void ProcessAction(const string &strAct, const vector<string> &vArgs);

		ScriptHost(string strScript, unsigned int uiFreq){
			g_pHost.reset(this);
			m_uiAVRFreq = uiFreq;
			LoadScript(strScript);
		}
		static shared_ptr<ScriptHost> g_pHost;
		static map<string, Scriptable*> m_clients;
		static vector<string> m_script;
		static unsigned int m_iLine, m_uiAVRFreq;
		static bool m_bStarted;
		typedef struct linestate_t{
			linestate_t(){strCtxt = "", iActID = 0, vArgs = {""}, iLine = 0, pClient = nullptr, isValid = false;};
			string strCtxt;
			unsigned int iActID;
			vector<string> vArgs;
			unsigned int iLine;
			Scriptable *pClient;
			bool isValid;
		}linestate_t;

		static int m_iTimeoutCycles, m_iTimeoutCount;

		static linestate_t m_lnState;

};