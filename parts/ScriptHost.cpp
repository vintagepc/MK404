

/*
	ScriptHost.cpp - Core handler responsible for handling
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

#include <ScriptHost.h>
#include <sstream>
#include <fstream>

map<string, IScriptable*> ScriptHost::m_clients;
vector<string> ScriptHost::m_script;
unsigned int ScriptHost::m_iLine, ScriptHost::m_uiAVRFreq;
ScriptHost::linestate_t ScriptHost::m_lnState = linestate_t();
shared_ptr<ScriptHost> ScriptHost::g_pHost;
ScriptHost::State ScriptHost::m_state = ScriptHost::State::Idle;
int ScriptHost::m_iTimeoutCycles = -1, ScriptHost::m_iTimeoutCount = 0;
bool ScriptHost::m_bQuitOnTimeout = false;


void ScriptHost::PrintScriptHelp()
{
	printf("Scripting options for the current context:\n");
	for (auto it=m_clients.begin();it!=m_clients.end();it++)
		it->second->PrintRegisteredActions();
}

void ScriptHost::LoadScript(const string &strFile)
{
	string strLn;
	ifstream fileIn(strFile);
	while (getline(fileIn, strLn))
	{
		if (!strLn.empty())
			m_script.push_back(strLn);
	}
	m_iLine = 0;
	// TODO... validate script...
	printf("ScriptHost: Loaded %s\n",strFile.c_str());
}

// Parse line in the format Context::Action(arg1, arg2,...)
bool ScriptHost::GetLineParts(const string &strLine, string &strCtxt, string& strAct, vector<string>&vArgs)
{
	int iCtxEnd = strLine.find("::");
	int iArgBegin = strLine.find('(');
	int iArgEnd = strLine.find(')');
	if (iCtxEnd == string::npos || iArgBegin == string::npos || iArgEnd == string::npos)
		return false;
	strCtxt = strLine.substr(0,iCtxEnd);
	strAct = strLine.substr(iCtxEnd+2, (iArgBegin - iCtxEnd)-2);
	string strTmp, strArgs = strLine.substr(iArgBegin+1, (iArgEnd-iArgBegin)-1);
	istringstream argsIn(strArgs);
	while (getline(argsIn, strTmp,','))
		vArgs.push_back(strTmp);
	return true;
}

IScriptable::LineStatus ScriptHost::ProcessAction(unsigned int ID, const vector<string> &vArgs)
{
	switch (ID)
	{
		case ActSetTimeoutMs:
		{
			int iTime = stoi(vArgs.at(0));
			m_iTimeoutCycles = iTime *(m_uiAVRFreq/1000);
			printf("ScriptHost::SetTimeoutMs changed to %d (%d cycles)\n",iTime,m_iTimeoutCycles);
			m_iTimeoutCount = 0;
			break;
		}
		case ActSetQuitOnTimeout:
		{
			m_bQuitOnTimeout = stoi(vArgs.at(0))!=0;
			break;
		}
	}
	return LineStatus::Finished;
}

bool ScriptHost::ValidateScript()
{
	vector<string> vArgs;
	string strCtxt, strAct;
	printf("Validating script...\n");
	bool bClean = true;
	auto fcnErr = [](const string &sMsg, const int iLine) { printf("ScriptHost: Validation failed: %s on line %d : %s\n",sMsg.c_str(), iLine, m_script.at(iLine).c_str());};
	for (int i=0; i<m_script.size(); i++)
	{
		vArgs.clear();
		if (!ScriptHost::GetLineParts(m_script.at(i),strCtxt,strAct,vArgs))
		{
			bClean = false;
			fcnErr("Parse error: Line is not of the form Context::Action([arg1,arg2,...])",i);
			continue;
		}
		if (m_clients.count(strCtxt)==0)
		{
			bClean = false;
			fcnErr("Unknown context " + strCtxt, i);
			string strCtxts = "Available contexts:";
			for (auto it=m_clients.begin(); it!=m_clients.end(); it++)
			{
				strCtxts += " " + it->first + ",";
			}
			strCtxts.pop_back();
			printf("%s\n",strCtxts.c_str());
			continue;
		}
		if (m_clients.at(strCtxt)->m_ActionIDs.count(strAct)==0)
		{
			bClean = false;
			fcnErr("Unknown action " + strCtxt + "::" + strAct,i);
			printf("Available actions:\n");
			m_clients.at(strCtxt)->PrintRegisteredActions();
			continue;
		}
		int ID = m_clients.at(strCtxt)->m_ActionIDs.at(strAct);
		const vector<ArgType> vArgTypes = m_clients.at(strCtxt)->m_ActionArgs.at(ID);
		if (vArgTypes.size()!=vArgs.size())
		{
			bClean = false;
			fcnErr("Argument count mismatch, expected "+to_string(vArgTypes.size()),i);
						m_clients.at(strCtxt)->PrintRegisteredActions();
			continue;
		}
		for (int j=0; j<vArgTypes.size(); j++)
		{
			if (!CheckArg(vArgTypes.at(j),vArgs.at(j)))
			{
				bClean = false;
				fcnErr("Conversion error, expected \"" + m_ArgToString.at(vArgTypes.at(j)) + "\" but could not convert \"" + vArgs.at(j) + "\"",i);
				continue;
			}
		}

	}
	printf("Script validation finished.\n");
	return bClean;
}

bool ScriptHost::CheckArg(const ArgType &type, const string &val)
{
	try
	{
		switch (type)
		{
			case ArgType::Int:
				stoi(val);
				return true;
			case ArgType::Float:
				stof(val);
				return true;
			case ArgType::Bool:
				stoi(val);
				return true;
		}
	}
	catch(const std::exception &e)
	{
		return false;
	}

}

void ScriptHost::ParseLine(unsigned int iLine)
{
	string strCtxt, strAct;
	m_lnState.isValid = false;
	m_lnState.vArgs.clear();
	bool bIsInternal = false;
	if (!ScriptHost::GetLineParts(m_script.at(iLine),strCtxt,strAct,m_lnState.vArgs))
		return;

	m_lnState.iLine = iLine;
	if(!m_clients.count(strCtxt) || m_clients.at(strCtxt)==nullptr)
		return;

	m_lnState.strCtxt = strCtxt;

	IScriptable *pClient = m_lnState.pClient = m_clients.at(strCtxt);

	if (!m_lnState.pClient->m_ActionIDs.count(strAct))
		return;

	int iID = m_lnState.iActID = pClient->m_ActionIDs[strAct];

	if (m_lnState.vArgs.size()!=pClient->m_ActionArgs.at(iID).size())
		return;

	m_lnState.isValid = true;
}

void ScriptHost::AddScriptable(string strName, IScriptable* src)
{
	if (m_clients.count(strName)==0)
	{
		m_clients[strName] = src;
	}
	else if (m_clients.at(strName)!=src)
	{
		int i=0;
		string strNew;
		printf("ScriptHost: NOTE: Duplicate context name (%s) with different pointer. Incrementing ID...\n", strName.c_str());
		while (i<10)
		{
			i++;
			strNew = strName + to_string(i);
			if (m_clients.count(strNew)==0)
			{
				m_clients[strNew] = src;
				src->SetName(strNew);
				return;
			}
			else if (m_clients.at(strNew) == src)
				return;
		};
		printf("ScriptHost: More than 10 duplicate identifiers. You should do something about that.\n");

	}
}

using LS = IScriptable::LineStatus;
void ScriptHost::OnAVRCycle()
{
	if (m_iLine>=m_script.size())
		return; // Done.

	if (m_lnState.iLine != m_iLine || m_state == State::Idle)
	{
		m_state = State::Running;
		printf("ScriptHost: Executing line %s\n",m_script.at(m_iLine).c_str());
		ParseLine(m_iLine);
	}

	if (m_lnState.isValid)
	{
		LS lsResult = m_lnState.pClient->ProcessAction(m_lnState.iActID,m_lnState.vArgs);
		switch (lsResult)
		{
			case LS::Finished:
				m_iLine++; // This line is done, mobe on.
				m_iTimeoutCount = 0;
				break;
			case LS::Error:
				printf("ScriptHost: Script FAILED on line %d\n",m_iLine);
				m_iLine = m_script.size(); // Error, end scripting.
				m_state = State::Error;
				return;
			case LS::Waiting:
				if(m_iTimeoutCycles>=0 && ++m_iTimeoutCount>m_iTimeoutCycles)
				{
					m_state = State::Timeout;
					if (m_bQuitOnTimeout)
					{
						printf("ScriptHost: Script TIMED OUT on %s. Quitting...\n",m_script.at(m_iLine).c_str());
						int ID = m_clients.at("Board")->m_ActionIDs.at("Quit");
						m_clients.at("Board")->ProcessAction(ID,{});
						m_iLine = m_script.size();
						return;
					}
					printf("ScriptHost: Script TIMED OUT on %s\n",m_script.at(m_iLine).c_str());
					m_iLine++;
					m_iTimeoutCount = 0;
				}

		}
		if (m_iLine==m_script.size())
		{
			printf("ScriptHost: Script FINISHED\n");
			m_state = State::Finished;
		}
	}
	else
	{
		printf("ScriptHost: ERROR: Invalid line/unrecognized command:%d %s\n",m_iLine,m_script.at(m_iLine).c_str());
		m_state = State::Error;
		m_iLine = m_script.size();
	}
}

const map<ArgType,string> IScriptable::m_ArgToString = {
	make_pair(ArgType::Bool,"bool"),
	make_pair(ArgType::Float,"float"),
	make_pair(ArgType::Int,"int"),
	make_pair(ArgType::String,"string"),
};
