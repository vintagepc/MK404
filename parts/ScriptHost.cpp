/*
	ScriptHost.cpp - Core handler responsible for handling
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

#include "ScriptHost.h"
#include "gsl-lite.hpp"
#include <GL/freeglut_std.h> // glut menus
#include <cstddef>
#include <exception>    // for exception
#include <fstream>      // IWYU pragma: keep
#include <iostream>
#include <sstream>		// IWYU pragma: keep
#include <utility>      // for make_pair, pair

std::map<std::string, IScriptable*> ScriptHost::m_clients;

std::vector<std::string> ScriptHost::m_script;
unsigned int ScriptHost::m_iLine, ScriptHost::m_uiAVRFreq;
std::map<std::string, int> ScriptHost::m_mMenuIDs;
std::map<unsigned,IScriptable*> ScriptHost::m_mMenuBase2Client;
std::map<std::string, unsigned> ScriptHost::m_mClient2MenuBase;
std::map<std::string, std::vector<std::pair<std::string,int>>> ScriptHost::m_mClientEntries;
ScriptHost::State ScriptHost::m_state = ScriptHost::State::Idle;
int ScriptHost::m_iTimeoutCycles = -1, ScriptHost::m_iTimeoutCount = 0;
bool ScriptHost::m_bQuitOnTimeout = false;
bool ScriptHost::m_bMenuCreated = false;
bool ScriptHost::m_bIsInitialized = false;
bool ScriptHost::m_bIsExecHold = false;
std::atomic_uint ScriptHost::m_uiQueuedMenu {0};

void ScriptHost::PrintScriptHelp(bool bMarkdown)
{
	if (bMarkdown)
	{
		std::cout << "# Scripting options for the selected printer:\n";
	}
	else
	{
		std::cout << "Scripting options for the current context:\n";
	}
	for (auto &client : m_clients)
	{
		client.second->PrintRegisteredActions(bMarkdown);
	}
}

void ScriptHost::LoadScript(const std::string &strFile)
{
	std::string strLn;
	std::ifstream fileIn(strFile);
	while (getline(fileIn, strLn))
	{
		if (strLn.empty() || strLn[0]=='#')
		{
			continue;
		}
		m_script.push_back(strLn);
	}
	m_iLine = 0;
	std::cout << "ScriptHost: Loaded " << m_script.size() << " lines from " << strFile << '\n';
}

bool ScriptHost::Init()
{
	GetHost()._Init();
	m_bIsInitialized = true;
	return true;
}

bool ScriptHost::Setup(const std::string &strScript,unsigned uiFreq)
{
	m_uiAVRFreq = uiFreq;
	if (!strScript.empty())
	{
		LoadScript(strScript);
	}
	return ValidateScript();
}

void ScriptHost::_Init()
{
	RegisterAction("SetTimeoutMs","Sets a timeout for actions that wait for an event",ActSetTimeoutMs,{ArgType::Int});
	RegisterAction("SetQuitOnTimeout","If 1, quits when a timeout occurs. Exit code will be non-zero.",ActSetQuitOnTimeout,{ArgType::Bool});
	RegisterAction("Log","Print the std::string to stdout",ActLog,{ArgType::String});
	m_clients[m_strName] = this;
}

// Parse line in the format Context::Action(arg1, arg2,...)
ScriptHost::LineParts_t ScriptHost::GetLineParts(const std::string &strLine)
{
	LineParts_t sParts;
	size_t iCtxEnd = strLine.find("::");
	size_t iArgBegin = strLine.find('(');
	size_t iArgEnd = strLine.find(')');
	std::vector<std::string> args;
	if (iCtxEnd == std::string::npos || iArgBegin == std::string::npos || iArgEnd == std::string::npos)
	{
		return sParts;
	}
	sParts.strCtxt = strLine.substr(0,iCtxEnd);
	sParts.strAct = strLine.substr(iCtxEnd+2, (iArgBegin - iCtxEnd)-2);
	std::string strTmp, strArgs = strLine.substr(iArgBegin+1, (iArgEnd-iArgBegin)-1);
	std::istringstream argsIn(strArgs);
	while (getline(argsIn, strTmp,','))
	{
		args.push_back(strTmp);
	}
	sParts.vArgs = args;
	sParts.isValid = true;
	return sParts;
}

IScriptable::LineStatus ScriptHost::ProcessAction(unsigned int ID, const std::vector<std::string> &vArgs)
{
	switch (ID)
	{
		case ActSetTimeoutMs:
		{
			int iTime = std::stoi(vArgs.at(0));
			m_iTimeoutCycles = iTime *(m_uiAVRFreq/1000);
			std::cout << "ScriptHost::SetTimeoutMs changed to " << iTime << " Ms (" << m_iTimeoutCycles << " cycles)\n";
			m_iTimeoutCount = 0;
			break;
		}
		case ActSetQuitOnTimeout:
		{
			m_bQuitOnTimeout = std::stoi(vArgs.at(0))!=0;
			break;
		}
		case ActLog:
		{
			std::cout << "ScriptLog: " << vArgs.at(0) << '\n';
			break;
		}
	}
	return LineStatus::Finished;
}

bool ScriptHost::ValidateScript()
{
	std::string strCtxt;
	std::cout << "Validating script...\n";
	bool bClean = true;
	auto fcnErr = [](const std::string &sMsg, const int iLine) { std::cout << "ScriptHost: Validation failed: "<< sMsg <<" on line " << iLine <<":" << m_script.at(iLine) << '\n';};
	for (size_t i=0; i<m_script.size(); i++)
	{
		LineParts_t sLine = ScriptHost::GetLineParts(m_script.at(i));
		strCtxt = sLine.strCtxt;
		if (!sLine.isValid)
		{
			bClean = false;
			fcnErr("Parse error: Line is not of the form Context::Action([arg1,arg2,...])",i);
			continue;
		}
		if (m_clients.count(strCtxt)==0)
		{
			bClean = false;
			fcnErr("Unknown context " + strCtxt, i);
			std::string strCtxts = "Available contexts:";
			for (auto &it: m_clients)
			{
				strCtxts += " " + it.first + ",";
			}
			strCtxts.pop_back();
			std::cout << strCtxts << '\n';
			continue;
		}
		if (m_clients.at(strCtxt)->m_ActionIDs.count(sLine.strAct)==0)
		{
			bClean = false;
			fcnErr(std::string("Unknown action ").append(strCtxt).append("::").append(sLine.strAct),i);
			std::cout << "Available actions:\n";
			m_clients.at(strCtxt)->PrintRegisteredActions();
			continue;
		}
		int ID = m_clients.at(strCtxt)->m_ActionIDs.at(sLine.strAct);
		const std::vector<ArgType> vArgTypes = m_clients.at(strCtxt)->m_ActionArgs.at(ID);
		if (vArgTypes.size()!=sLine.vArgs.size())
		{
			bClean = false;
			fcnErr("Argument count mismatch, expected "+ std::to_string(vArgTypes.size()),i);
						m_clients.at(strCtxt)->PrintRegisteredActions();
			continue;
		}
		for (size_t j=0; j<vArgTypes.size(); j++)
		{
			if (!CheckArg(vArgTypes.at(j),sLine.vArgs.at(j)))
			{
				bClean = false;
				fcnErr("Conversion error, expected \"" + GetArgTypeNames().at(vArgTypes.at(j)) + "\" but could not convert \"" + sLine.vArgs.at(j) + "\"",i);
				continue;
			}
		}

	}
	std::cout << "Script validation finished.\n";
	return bClean;
}

// Called from the execution context to process the menu action.
void ScriptHost::DispatchMenuCB()
{
	if (m_uiQueuedMenu !=0)
	{
		unsigned iID = m_uiQueuedMenu;
		m_uiQueuedMenu.store(0);
		IScriptable *pClient = m_mMenuBase2Client[(iID - iID%100)];
		pClient->ProcessMenu(iID%100);

	}
}

// Dispatches menu callbacks to the client.
void ScriptHost::MenuCB(int iID)
{
	//printf("Menu CB %d\n",iID);
	m_uiQueuedMenu.store(iID);
}

void ScriptHost::CreateRootMenu(int iWinID)
{
	m_bMenuCreated = true;
	if (m_mMenuIDs.count("ScriptHost")!=0)
	{
		std::cerr << "Attempted to create a new root menu when one already exists. Ignoring...\n";
		return;
	}

	int iRootID = glutCreateMenu(ScriptHost::MenuCB);
	m_mMenuIDs["ScriptHost"] =  iRootID;
	glutSetWindow(iWinID);
	glutAttachMenu(m_mMenuIDs["ScriptHost"]);
	glutAttachMenu(GLUT_MIDDLE_BUTTON);

	// Run through the list of clients and add ones that registered before GLUT was ready:
	for (auto it = m_mMenuIDs.begin(); it!=m_mMenuIDs.end(); it++)
	{
		if (it->second != 0)
		{
			continue;
		}
		auto str = it->first;
		int iID = glutCreateMenu(ScriptHost::MenuCB);
		if (m_mClientEntries.count(str)==0)
		{
			glutAddMenuEntry("No options",m_mClient2MenuBase.at(str));
		}
		else
		{
			while (m_mClientEntries.at(str).size()>0)
			{
				auto entry = m_mClientEntries.at(str).back();
				glutAddMenuEntry(entry.first.c_str(), entry.second);
				m_mClientEntries.at(str).pop_back();
			}
		}

		m_mMenuIDs[str] = iID;
		glutSetMenu(iRootID);
		glutAddSubMenu(it->first.c_str(),iID);
	}
}

bool ScriptHost::CheckArg(const ArgType &type, const std::string &val)
{
	try
	{
		switch (type)
		{
			case ArgType::Int:
				std::stoi(val);
				return true;
			case ArgType::Float:
				std::stof(val);
				return true;
			case ArgType::Bool:
				std::stoi(val);
				return true;
			case ArgType::String:
				return true;
			case ArgType::uint32:
				std::stoul(val);
				return true;
		}
	}
	catch(const std::exception &e)
	{
		return false;
	}
	return false;

}

void ScriptHost::ParseLine(unsigned int iLine)
{
	GetLineState().isValid = false;
	LineParts_t sLine = ScriptHost::GetLineParts(m_script.at(iLine));
	if (!sLine.isValid)
	{
		std::cout << "Failed to get parts\n";
		return;
	}

	GetLineState().iLine = iLine;
	if(!m_clients.count(sLine.strCtxt) || m_clients.at(sLine.strCtxt)==nullptr)
	{
		std::cout << "No client\n";
		return;
	}

	GetLineState().strCtxt = sLine.strCtxt;
	GetLineState().vArgs = sLine.vArgs;

	IScriptable *pClient = GetLineState().pClient = m_clients.at(sLine.strCtxt);

	if (!GetLineState().pClient->m_ActionIDs.count(sLine.strAct))
	{
		std::cout << "No action\n";
		return;
	}

	int iID = GetLineState().iActID = pClient->m_ActionIDs[sLine.strAct];

	if (GetLineState().vArgs.size()!=pClient->m_ActionArgs.at(iID).size())
	{
		std::cout << "Arg count mismatch\n";
		return;
	}
	GetLineState().isValid = true;
}

void ScriptHost::AddSubmenu(IScriptable *src)
{
	std::string strName = src->GetName();
	if (m_bMenuCreated)
	{
		std::cout << "Adding a menu entry after GLUT is up... TODO\n";
	}
	else if (!m_mMenuIDs.count(strName)) // GLUT isn't up yet, queue it for later.
	{
		m_mMenuIDs[strName] = 0;
		unsigned uiBase = 100U*m_mMenuIDs.size();
		m_mMenuBase2Client[uiBase] = src;
		m_mClient2MenuBase[strName] = uiBase;
		//printf("Registered %s with menu base %u\n",strName.c_str(),uiBase);
	}
}

void ScriptHost::AddMenuEntry(const std::string &strName, unsigned uiID, IScriptable* src)
{
	Expects(uiID<100); //NOLINT - complaint is in system include file
	auto strClient = src->GetName();
	unsigned uiBase = m_mClient2MenuBase.at(strClient);
	m_mClientEntries[strClient].push_back({strName, uiBase + uiID});

}


void ScriptHost::AddScriptable(const std::string &strName, IScriptable* src)
{
	if (m_clients.count(strName)==0)
	{
		m_clients[strName] = src;
		AddSubmenu(src);
	}
	else if (m_clients.at(strName)!=src)
	{
		int i=0;
		std::string strNew;
		std::cout << "ScriptHost: NOTE: Duplicate context name (" << strName << ") with different pointer. Incrementing ID...\n";
		while (i<10)
		{
			i++;
			strNew = strName + std::to_string(i);
			if (m_clients.count(strNew)==0)
			{
				m_clients[strNew] = src;
				src->SetName(strNew);
				AddSubmenu(src);
				return;
			}
			else if (m_clients.at(strNew) == src)
			{
				return;
			}
		};
		std::cerr << "ScriptHost: More than 10 duplicate identifiers. You should do something about that.\n";

	}
}

using LS = IScriptable::LineStatus;
void ScriptHost::OnAVRCycle()
{
	if (m_iLine>=m_script.size())
	{
		return; // Done.
	}
	if (GetLineState().iLine != m_iLine || m_state == State::Idle)
	{
		m_state = State::Running;
		std::cout << "ScriptHost: Executing line " << m_script.at(m_iLine) << "\n";
		ParseLine(m_iLine);
	}
	if (GetLineState().isValid)
	{
		LS lsResult = GetLineState().pClient->ProcessAction(GetLineState().iActID,GetLineState().vArgs);
		switch (lsResult)
		{
			case LS::Finished:
				if (m_bIsExecHold)
				{
					m_bIsExecHold = false;
					if (m_clients.count("Board") && m_clients.at("Board")->m_ActionIDs.count("Resume"))
					{
						int ID = m_clients.at("Board")->m_ActionIDs.at("Resume");
						LS lsUnpause = m_clients.at("Board")->ProcessAction(ID,{});
						if (lsUnpause !=LS::Finished)
						{
							std::cerr << "Client failed to resume after ExecHold - ID " << std::to_string(ID) << '\n';
						}
					}
					else
					{
						std::cerr << "Failed to resume after ExecHold!\n";
					}

				}
				m_iLine++; // This line is done, mobe on.
				m_iTimeoutCount = 0;
				break;
			case LS::Unhandled:
				std::cout << "ScriptHost: Unhandled action, considering this an error.\n";
				/* FALLTHRU */
			case LS::Error:
			{
				std::cout << "ScriptHost: Script FAILED on line " << m_iLine << '\n';
				m_state = State::Error;
				int ID = m_clients.at("Board")->m_ActionIDs.at("Quit");
				m_clients.at("Board")->ProcessAction(ID,{});
				m_iLine = m_script.size(); // Error, end scripting.
				return;
			}
			case LS::HoldExec: // like waiting, but pauses board.
			{
				// TODO (vintagepc) : clean this up and also handle board1.
				if (m_clients.count("Board") && m_clients.at("Board")->m_ActionIDs.count("Pause"))
				{
					int ID = m_clients.at("Board")->m_ActionIDs.at("Pause");
					m_clients.at("Board")->ProcessAction(ID,{});
					m_bIsExecHold = true;
				}
			}
			/* FALLTHRU */
			case LS::Waiting:
			{
				if(m_iTimeoutCycles>=0 && ++m_iTimeoutCount<=m_iTimeoutCycles)
				{
					break;
				}
				else
				{
					m_state = State::Timeout;
				}
			}
			/* FALLTHRU */
			case LS::Timeout:
			{
				m_state = State::Timeout;
				if (m_bQuitOnTimeout)
				{
					std::cout << "ScriptHost: Script TIMED OUT on " << m_script.at(m_iLine) << ". Quitting...\n";
					int ID = m_clients.at("Board")->m_ActionIDs.at("Resume");
					m_clients.at("Board")->ProcessAction(ID,{});
					ID = m_clients.at("Board")->m_ActionIDs.at("Quit");
					m_clients.at("Board")->ProcessAction(ID,{});
					m_iLine = m_script.size();
					return;
				}
				std::cout << "ScriptHost: Script TIMED OUT on #" << m_iLine << ": " << m_script.at(m_iLine) << '\n';
				m_iLine++;
				m_iTimeoutCount = 0;
			}
			break;
			default:
				break;

		}
		if (m_iLine==m_script.size())
		{
			std::cout << "ScriptHost: Script FINISHED\n";
			m_state = State::Finished;
		}
	}
	else
	{
		std::cout << "ScriptHost: ERROR: Invalid line/unrecognized command: " << m_iLine << ":" << m_script.at(m_iLine) << '\n';
		m_state = State::Error;
		m_iLine = m_script.size();
	}
}
