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


#include <stdio.h>        // for fprintf, stderr
#include <map>            // for map
#include <memory>         // for operator!=, shared_ptr
#include <string>         // for string
#include <vector>         // for vector
#include "IScriptable.h"  // for ArgType, ArgType::Bool, ArgType::Int, IScri...

using namespace std;

class ScriptHost: public IScriptable
{
    public:
		static shared_ptr<ScriptHost> Get()
		{
			return g_pHost;
		}

		inline static bool IsInitialized()
		{
			return g_pHost!=nullptr;
		}
		static bool Init(std::string strFile, unsigned int uiFreq)
		{
			if (g_pHost!=nullptr)
			{
				fprintf(stderr,"ERROR: Duplicate initialization attempt for scripthost!\n");
				return false;
			}
			new ScriptHost(strFile,uiFreq);
			return ValidateScript();
		}

		static void AddScriptable(string strName, IScriptable* src);

		static void AddMenuEntry(const string &strName, uint uiID, IScriptable* src);

		static inline bool IsRegistered(string strName)
		{
			return m_clients.count(strName)!=0;
		}

		static void CreateRootMenu(int iWinID);

		static void MenuCB(int iID);

		static void PrintScriptHelp(bool bMarkdown);

		static void OnAVRCycle();

		enum class State
		{
			Finished, // First because 0 return code is OK.
			Idle,
			Running,
			Timeout,
			Error
		};

		static inline State GetState(){ return m_state;}

    private:
		static bool ValidateScript();
		static void LoadScript(const string &strScript);
		static void ParseLine(unsigned int iLine);
		static bool GetLineParts(const string &strLine, string &strCtxt, string& strAct, vector<string>&vArgs);
		static bool CheckArg(const ArgType &type, const string &val);

		static void AddSubmenu(IScriptable *src);

		//We can't register ourselves as a scriptable so just fake it with a processing func.
		LineStatus ProcessAction(unsigned int ID, const vector<string> &vArgs) override;

		ScriptHost(string strScript, unsigned int uiFreq):IScriptable("ScriptHost"){
			g_pHost.reset(this);
			RegisterAction("SetTimeoutMs","Sets a timeout for actions that wait for an event",ActSetTimeoutMs,{ArgType::Int});
			RegisterAction("SetQuitOnTimeout","If 1, quits when a timeout occurs. Exit code will be non-zero.",ActSetQuitOnTimeout,{ArgType::Bool});
			m_clients[m_strName] = this;
			m_uiAVRFreq = uiFreq;
			if (!strScript.empty())
				LoadScript(strScript);
		}
		static shared_ptr<ScriptHost> g_pHost;
		static map<string, IScriptable*> m_clients;
		static map<string, int> m_mMenuIDs;
		static map<string, uint> m_mClient2MenuBase;
		static map<uint, IScriptable*> m_mMenuBase2Client;
		static map<string, vector<pair<string,int>>> m_mClientEntries; // Stores client entries for when GLUT is ready.
		static vector<string> m_script;
		static unsigned int m_iLine, m_uiAVRFreq;
		static ScriptHost::State m_state;
		static bool m_bQuitOnTimeout;

		typedef struct linestate_t{
			linestate_t(){strCtxt = "", iActID = 0, vArgs = {""}, iLine = 0, pClient = nullptr, isValid = false;};
			string strCtxt;
			unsigned int iActID;
			vector<string> vArgs;
			unsigned int iLine;
			IScriptable *pClient;
			bool isValid;
		}linestate_t;

		enum Actions
		{
			ActSetTimeoutMs,
			ActSetQuitOnTimeout
		};

		static int m_iTimeoutCycles, m_iTimeoutCount;

		static linestate_t m_lnState;

};
