/*
	IScriptable.h - Scriptable base interface. Needs to be separate from
	Scriptable because ScriptHost must also be scriptable, but would
	create a cyclic header if it was.
	So ScriptHost will be IScriptable but skips registering itself with itself
	 and needs to do that manually.

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

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
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
	Float,
	uint32
};

class IScriptable
{
	friend Scriptable;


	friend ScriptHost;
	friend TelemetryHost;
    public:
		explicit IScriptable(std::string strName):m_strName(std::move(strName)){}
        virtual ~IScriptable() = default;
	enum class LineStatus
	{
		Error,
		Timeout,
		Waiting,
		Running,
		Finished,
		Unhandled
	};


    protected:
		inline IScriptable::LineStatus IssueLineError(const std::string &msg)
		{
			std::cerr << m_strName << "ERROR: " << msg << '\n';
			return LineStatus::Error;
		}

		virtual LineStatus ProcessAction(unsigned int /*iAction*/, const std::vector<std::string> &/*args*/)
		{
			return IssueLineError(" Has registered actions but does not have an action handler!");
		}

		// Processes the menu callback. By default, will try the script handler for no-arg actions.
		// If this is NOT what you want, overload this in your class.
		virtual void ProcessMenu(unsigned iAction)
		{
			//printf("m_Act: %u\n", (unsigned)m_ActionArgs.count(iAction));
			if (m_ActionArgs.count(iAction)==0 || m_ActionArgs.at(iAction).size()==0) // If no args needed or it wasn't registered, try the script handler.
			{
				auto LSResult = ProcessAction(iAction,{});
				if (LSResult != LineStatus::Error && LSResult != LineStatus::Unhandled)
				{
					return;
				}
			}
			std::cerr << "Programmer error: " << m_strName << " has registered menu items but no valid handler!\n";
		}

		void SetName(const std::string &strName)
		{
			if (m_bRegistered)
			{
				std::cerr << "ERROR: Tried to change a Scriptable object's name after it has already registered.\n";
			}
			else
			{
				m_strName = strName;
			}
		}

		// Returns the name. Used by, e.g. TelHost for consistency.
		inline std::string GetName() {return m_strName;}

		// Prints help text for this Scriptable
		void PrintRegisteredActions(bool bMarkdown = false)
		{
			std::cout << (bMarkdown?"### ":"\t") << m_strName << "::\n";
			for (auto &ActID: m_ActionIDs)
			{
				unsigned int ID = ActID.second;
				std::string strArgFmt = ActID.first;
				strArgFmt.push_back('(');
				if (m_ActionArgs[ID].size()>0)
				{
					for (auto &Arg : m_ActionArgs[ID])
					{
						strArgFmt += GetArgTypeNames().at(Arg) + ", ";
					}
					strArgFmt[strArgFmt.size()-2] = ')';
				}
				else
				{
					strArgFmt.push_back(')');
				}
				if (bMarkdown)
				{
					std::cout << " - `" << std::setw(30) << strArgFmt << "` - `" << m_mHelp.at(ID) << "`\n";
				}
				else
				{
					std::cout << "\t\t" << std::setw(30) << strArgFmt << m_mHelp.at(ID) << '\n';
				}
			}
		}
		// Registers a new no-argument Scriptable action with the given function, description, and an ID that will be
		// provided in ProcessAction. This lets you set up an internal enum and switch() on actions
		// instead of needing to make a string-comparison if-else conditional.
		inline virtual bool RegisterAction(const std::string &strAct, const std::string& strDesc, unsigned int ID)
		{
			if (m_ActionIDs.count(strAct)>0)
			{
				std::cerr << "ERROR: Attempted to register duplicate action handler " << m_strName << "::" << strAct;
				return false;
			}
			m_ActionIDs[strAct] = ID;
			m_mHelp[ID] = strDesc;
			m_ActionArgs[ID].clear();
			return true;
		}

		// Registers a scriptable action Name::strAct(), help description strDesc, internal ID, and a vector of argument types.
		// The types are (currently) for display only but the count is used to sanity-check lines before passing them to you in ProcessAction.
		inline void RegisterAction(const std::string &strAct, const std::string& strDesc, unsigned int ID, const std::vector<ArgType>& vTypes)
		{
			if (!RegisterAction(strAct,strDesc, ID))
			{
				return;
			}
			m_ActionArgs[ID] = vTypes;
		}

		static const std::map<ArgType,std::string>& GetArgTypeNames()
		{
			static const std::map<ArgType,std::string> m {
				{ArgType::Bool,"bool"},
				{ArgType::Float,"float"},
				{ArgType::Int,"int"},
				{ArgType::String,"string"},
				{ArgType::uint32,"uint32"}
			};
			return m;
		}

    private:
		std::string m_strName;
		bool m_bRegistered = false;
		std::map<unsigned int, std::vector<ArgType>> m_ActionArgs;
		std::map<std::string, unsigned int> m_ActionIDs;
		std::map<unsigned int,std::string> m_mHelp;
};
