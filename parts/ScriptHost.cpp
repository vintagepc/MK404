

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
#include <Scriptable.h>
#include <sstream>
#include <fstream>

map<string, Scriptable*> ScriptHost::m_clients;
vector<string> ScriptHost::m_script;
unsigned int ScriptHost::m_iLine, ScriptHost::m_uiAVRFreq;
ScriptHost::linestate_t ScriptHost::m_lnState = linestate_t();
shared_ptr<ScriptHost> ScriptHost::g_pHost;
bool ScriptHost::m_bStarted = false;
int ScriptHost::m_iTimeoutCycles = -1, ScriptHost::m_iTimeoutCount = 0;


void ScriptHost::PrintScriptHelp()
{
	printf("Scripting options for the current context:\n");
	printf("\tScriptHost::\n\t\tSetTimeoutMs(int)  Sets the timeout on actions that wait for an event. 'time' is relative to the AVR, e.g. based off frequency)\n");
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

void ScriptHost::ProcessAction(const string &strAct, const vector<string> &vArgs)
{
	if (strAct.compare("SetTimeoutMs")==0)
	{
		int iTime = stoi(vArgs.at(0));
		m_iTimeoutCycles = iTime *(m_uiAVRFreq/1000);
		printf("ScriptHost::SetTimeoutMs changed to %d (%d cycles)\n",iTime,m_iTimeoutCycles);
		m_iTimeoutCount = 0;
	}
}

void ScriptHost::ParseLine(unsigned int iLine)
{
	string strCtxt, strAct;
	m_lnState.isValid = false;
	m_lnState.vArgs.clear();
	bool bIsInternal = false;
	do {
		if (!ScriptHost::GetLineParts(m_script.at(iLine),strCtxt,strAct,m_lnState.vArgs))
			return;
		if(strCtxt.compare("ScriptHost")==0)
		{
			ProcessAction(strAct,m_lnState.vArgs);
			m_iLine++;
			iLine = m_iLine;
			bIsInternal = true;
			m_lnState.vArgs.clear();
		}
		else
			bIsInternal = false;
	} while (bIsInternal);

	m_lnState.iLine = iLine;
	if(!m_clients.count(strCtxt) || m_clients.at(strCtxt)==nullptr)
		return;

	m_lnState.strCtxt = strCtxt;

	Scriptable *pClient = m_lnState.pClient = m_clients.at(strCtxt);

	if (!m_lnState.pClient->m_ActionIDs.count(strAct))
		return;

	int iID = m_lnState.iActID = pClient->m_ActionIDs[strAct];

	if (m_lnState.vArgs.size()!=pClient->m_ActionArgs.at(iID).size())
		return;

	m_lnState.isValid = true;
}

void ScriptHost::AddScriptable(string strName, Scriptable* src)
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

using LS = Scriptable::LineStatus;
void ScriptHost::OnAVRCycle()
{
	if (m_iLine>=m_script.size())
		return; // Done.

	if (m_lnState.iLine != m_iLine || !m_bStarted)
	{
		m_bStarted = true;
		ParseLine(m_iLine);
		printf("ScriptHost: Executing line %s\n",m_script.at(m_iLine).c_str());
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
				break;
			case LS::Waiting:
				if(m_iTimeoutCycles>=0 && ++m_iTimeoutCount>m_iTimeoutCycles)
				{
					printf("ScriptHost: Script TIMED OUT on %s\n",m_script.at(m_iLine).c_str());
					m_iLine++;
					m_iTimeoutCount = 0;
				}

		}
		if (m_iLine>m_script.size())
			printf("ScriptHost: Script FINISHED\n");
	}
}
