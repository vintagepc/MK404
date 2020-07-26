/*
	IScriptable.h - Scriptable base interface. Needs to be separate from
	Scriptable because ScriptHost must also be scriptable, but would
	create a cyclic header if it was.
	So ScriptHost will be IScriptable but skips registering itself with itself
	 and needs to do that manually.

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
class Scriptable;
class ScriptHost;
class TelemetryHost;

// Argument type options. Note, the const string defs live in ScriptHost.cpp, update those too!
enum class ArgType
{
	Int,
	String,
	Bool,
	Float


};

class IScriptable
{
	friend Scriptable;
	friend ScriptHost;
	friend TelemetryHost;
    public:
		IScriptable(const string &strName):m_strName(strName){};

	enum class LineStatus
	{
		Error,
		Waiting,
		Running,
		Finished,
		Unhandled
	};


    protected:
		inline IScriptable::LineStatus IssueLineError(const string &msg)
		{
			fprintf(stderr,"%s ERROR: %s\n",m_strName.c_str(), msg.c_str());
			return LineStatus::Error;
		}

		virtual LineStatus ProcessAction(unsigned int iAction, const vector<string> &args)
		{
			return IssueLineError(" Has registered actions but does not have an action handler!");
		}

		// Processes the menu callback. By default, will try the script handler for no-arg actions.
		// If this is NOT what you want, overload this in your class.
		virtual void ProcessMenu(uint iAction)
		{
			if (!m_ActionArgs.count(iAction) || m_ActionArgs.at(iAction).size()==0) // If no args needed or it wasn't registered, try the script handler.
			{
				auto LSResult = ProcessAction(iAction,{});
				if (LSResult != LineStatus::Error || LSResult != LineStatus::Unhandled)
					return;
			}
			fprintf(stderr, "Programmer error: %s has registered menu items but no valid handler!\n",m_strName.c_str());
		}

		void SetName(const string &strName)
		{
			if (m_bRegistered)
				printf("ERROR: Tried to change a Scriptable object's name after it has already registered.\n");
			else
				m_strName = strName;
		}

		// Returns the name. Used by, e.g. TelHost for consistency.
		inline string GetName() {return m_strName;}

		// Prints help text for this Scriptable
		void PrintRegisteredActions(bool bMarkdown = false)
		{
			printf("%s%s::\n",bMarkdown?"### ":"\t", m_strName.c_str());
			for (auto it=m_ActionIDs.begin();it!=m_ActionIDs.end();it++)
			{
				unsigned int ID = it->second;
				string strArgFmt = it->first;
				strArgFmt.push_back('(');
				if (m_ActionArgs[ID].size()>0)
				{
					for (size_t i=0; i<m_ActionArgs[ID].size(); i++)
						strArgFmt += m_ArgToString.at(m_ActionArgs.at(ID).at(i)) + ", ";
					strArgFmt[strArgFmt.size()-2] = ')';
				}
				else
					strArgFmt.push_back(')');
				if (bMarkdown)
					printf(" - `%-30s` - `%s`\n",strArgFmt.c_str(), m_mHelp.at(ID).c_str());
				else
					printf("\t\t%-30s%s\n",strArgFmt.c_str(), m_mHelp.at(ID).c_str());
			}
		}
		// Registers a new no-argument Scriptable action with the given function, description, and an ID that will be
		// provided in ProcessAction. This lets you set up an internal enum and switch() on actions
		// instead of needing to make a string-comparison if-else conditional.
		inline virtual bool RegisterAction(const string &strAct, const string& strDesc, unsigned int ID)
		{
			if (m_ActionIDs.count(strAct)>0)
			{
				fprintf(stderr,"ERROR: Attempted to register duplicate action handler %s::%s\n",m_strName.c_str(),strAct.c_str());
				return false;
			}
			m_ActionIDs[strAct] = ID;
			m_mHelp[ID] = strDesc;
			m_ActionArgs[ID].clear();
			return true;
		}

		// Registers a scriptable action Name::strAct(), help description strDesc, internal ID, and a vector of argument types.
		// The types are (currently) for display only but the count is used to sanity-check lines before passing them to you in ProcessAction.
		inline void RegisterAction(const string &strAct, const string& strDesc, unsigned int ID, const vector<ArgType>& vTypes)
		{
			if (!RegisterAction(strAct,strDesc, ID))
				return;
			m_ActionArgs[ID] = vTypes;
		}

    private:
		string m_strName;
		bool m_bRegistered = false;
		map<unsigned int, vector<ArgType>> m_ActionArgs;
		map<string, unsigned int> m_ActionIDs;
		map<unsigned int,string> m_mHelp;
		static const map<ArgType,string> m_ArgToString;

};
