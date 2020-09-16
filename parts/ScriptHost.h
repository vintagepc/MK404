/*
	ScriptHost.h - Core handler responsible for handling
	scripting actions.

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


#include "IScriptable.h"  // for ArgType, ArgType::Bool, ArgType::Int, IScri...

#include <atomic>         // for atomic_uint
#include <map>            // for map
#include <string>         // for std::string
#include <utility>        // for pair
#include <vector>         // for vector

class ScriptHost: public IScriptable
{
    public:
		inline static bool IsInitialized()
		{
			return m_bIsInitialized;
		}
		static bool Init()
		{
			GetHost()._Init();
			m_bIsInitialized = true;
			return true;
		}

		static bool Setup(const std::string &strScript,unsigned uiFreq)
		{
			m_uiAVRFreq = uiFreq;
			if (!strScript.empty())
			{
				LoadScript(strScript);
			}
			return ValidateScript();
		}

		static void AddScriptable(const std::string &strName, IScriptable* src);

		static void AddMenuEntry(const std::string &strName, unsigned uiID, IScriptable* src);

		static inline bool IsRegistered(const std::string &strName)
		{
			return m_clients.count(strName)!=0;
		}

		static void CreateRootMenu(int iWinID);

		static void DispatchMenuCB();

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

		using LineParts_t = struct
		{
			bool isValid {false};
			std::string strCtxt {""};
			std::string strAct {""};
			std::vector<std::string> vArgs {};
		};

		static bool ValidateScript();
		static void LoadScript(const std::string &strScript);
		static void ParseLine(unsigned int iLine);
		static LineParts_t GetLineParts(const std::string &strLine);
		static bool CheckArg(const ArgType &type, const std::string &val);

		static void AddSubmenu(IScriptable *src);

		//We can't register ourselves as a scriptable so just fake it with a processing func.
		LineStatus ProcessAction(unsigned int ID, const std::vector<std::string> &vArgs) override;

		ScriptHost():IScriptable("ScriptHost"){
		}

		void _Init()
		{
			RegisterAction("SetTimeoutMs","Sets a timeout for actions that wait for an event",ActSetTimeoutMs,{ArgType::Int});
			RegisterAction("SetQuitOnTimeout","If 1, quits when a timeout occurs. Exit code will be non-zero.",ActSetQuitOnTimeout,{ArgType::Bool});
			RegisterAction("Log","Print the std::string to stdout",ActLog,{ArgType::String});
			m_clients[m_strName] = this;
		}

		static ScriptHost& GetHost()
		{
			static ScriptHost h;
			return h;
		}


		using linestate_t = struct{
			std::string strCtxt {""};
			unsigned int iActID {0};
			std::vector<std::string> vArgs {};
			unsigned int iLine {0};
			IScriptable *pClient {nullptr};
			bool isValid {false};
		};

		inline static linestate_t& GetLineState()
		{
			static linestate_t state;
			return state;
		}

		static std::map<std::string, IScriptable*> m_clients;
		static std::map<std::string, int> m_mMenuIDs;
		static std::map<std::string, unsigned> m_mClient2MenuBase;
		static std::map<unsigned, IScriptable*> m_mMenuBase2Client;
		static std::map<std::string, std::vector<std::pair<std::string,int>>> m_mClientEntries; // Stores client entries for when GLUT is ready.
		static std::vector<std::string> m_script;
		static unsigned int m_iLine, m_uiAVRFreq;
		static ScriptHost::State m_state;
		static bool m_bQuitOnTimeout;
		static bool m_bMenuCreated;
		static bool m_bIsInitialized;
		static bool m_bIsExecHold;

		static std::atomic_uint m_uiQueuedMenu;





		enum Actions
		{
			ActSetTimeoutMs,
			ActSetQuitOnTimeout,
			ActLog
		};



		static int m_iTimeoutCycles, m_iTimeoutCount;

};
