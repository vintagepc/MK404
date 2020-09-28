/*
	TelemetryHost.cpp - Wrangler for VCD traces and scripted telemetry

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

#include "TelemetryHost.h"
#include "sim_vcd_file.h"  // for avr_vcd_add_signal
#include <algorithm>       // for find
#include <cstring>
#include <iomanip>
#include <iostream>
#include <utility>

TelemetryHost::TelemetryHost():Scriptable("TelHost"),IKeyClient()
{
	memset(&m_trace, 0, sizeof(m_trace));
#ifdef __CYGWIN__
	std::cout << "Cygwin detected - skipping TelHost action registration...\n";
#else
	// Sorry, this segfaults on win32 for some reason...
	RegisterAction("WaitFor","Waits for a specified telemetry value to occur",ActWaitFor, {ArgType::String,ArgType::uint32});
	RegisterAction("WaitForGT","Waits for a specified telemetry value to be greater than specified",ActWaitForGT, {ArgType::String,ArgType::uint32});
	RegisterAction("WaitForLT","Waits for a specified telemetry value to be less than specified",ActWaitForLT, {ArgType::String,ArgType::uint32});
	RegisterActionAndMenu("StartTrace", "Starts the telemetry trace. You must have set a category or set of items with the -t option",ActStartTrace);
	RegisterActionAndMenu("StopTrace", "Stops a running telemetry trace.",ActStopTrace);
#endif
	RegisterKeyHandler('+',"Start VCD trace");
	RegisterKeyHandler('-',"Stop VCD trace");
}

void TelemetryHost::Init(avr_t *pAVR, const std::string &strVCDFile, uint32_t uiRateUs)
{
	_Init(pAVR, this);
	avr_vcd_init(m_pAVR,strVCDFile.c_str(),&m_trace,uiRateUs);
}

void TelemetryHost::AddTrace(avr_irq_t *pIRQ, std::string strName, TelCats vCats, uint8_t uiBits)
{
	bool bShouldAdd = false;
	// Check categories.
	for (auto &vCat : vCats)
	{
		if (find(m_VLoglst.begin(), m_VLoglst.end(), vCat)!=m_VLoglst.end())
		{
			bShouldAdd = true;
			break;
		}
	}
	strName+= "_";
	strName.append(pIRQ->name);
	// Check explicit names
	for (auto &sName : m_vsNames)
	{
		if (strName.rfind(sName,0)==0)
		{
			bShouldAdd = true;
			break;
		}
	}

	if (bShouldAdd)
	{
		std::cout << "Telemetry: Added trace " << strName << '\n';
		avr_vcd_add_signal(&m_trace, pIRQ, uiBits, strName.c_str());
	}
	if (!m_mIRQs.count(strName))
	{
		m_mIRQs[strName] = pIRQ;
		m_mCatsByName[strName] = vCats;
		for(auto &vCat : vCats)
		{
			m_mNamesByCat[vCat].push_back(strName);
		}
	}
	else
	{
		std::cerr << "ERROR: Trying to add the same IRQ "<<  strName <<" to a VCD trace multiple times!\n";
	}
}

void TelemetryHost::OnKeyPress(const Key& key)
{
	switch (key)
	{
		case '+':
			TelemetryHost::GetHost().StartTrace();
			std::cout << "Enabled VCD trace." << '\n';
			break;
		case '-':
			TelemetryHost::GetHost().StopTrace();
			std::cout << "Stopped VCD trace" << '\n';
			break;
	}
}

void TelemetryHost::SetCategories(const std::vector<std::string> &vsCats)
{
	for (auto &sCat : vsCats)
	{
		if (!m_mStr2Cat.count(sCat))
		{
			m_vsNames.push_back(sCat); // Save non-category for name check later.
		}
		else if (find(m_VLoglst.begin(), m_VLoglst.end(), m_mStr2Cat.at(sCat))==m_VLoglst.end())
		{
			m_VLoglst.push_back(m_mStr2Cat.at(sCat));
		}
	}
}

Scriptable::LineStatus TelemetryHost::ProcessAction(unsigned int iAct, const std::vector<std::string> &vArgs)
{
	switch (iAct)
	{
		case ActWaitFor:
		case ActWaitForGT:
		case ActWaitForLT:
		{
			if (m_pCurrentIRQ == nullptr)
			{
				if (!m_mIRQs.count(vArgs.at(0)))
				{
					return IssueLineError("Asked to wait for telemetry " + vArgs.at(0) + " but it was not found");
				}
				else
				{
					m_pCurrentIRQ = m_mIRQs[vArgs.at(0)];
					//uint32_t tmpval = stoul(vArgs.at(1));
//					// if (tmpval!=m_uiMatchVal)
						// printf("WF: %08x (%u) //  %08x (%u)\n",tmpval,tmpval,m_pCurrentIRQ->value, m_pCurrentIRQ->value);
					m_uiMatchVal = stoul(vArgs.at(1));

				}
			}
			bool bMatch = false;
			if (iAct==ActWaitForLT)
			{
				bMatch = m_pCurrentIRQ->value < m_uiMatchVal;
			}
			else if (iAct == ActWaitForGT)
			{
				bMatch = m_pCurrentIRQ->value > m_uiMatchVal;
			}
			else
			{
				bMatch = m_pCurrentIRQ->value == m_uiMatchVal;
			}
			if (bMatch)
			{
				m_pCurrentIRQ = nullptr;
				return LineStatus::Finished;
			}
			else
			{
				return LineStatus::Waiting;
			}
		}
		case ActStartTrace:
		{
			StartTrace();
			return LineStatus::Finished;
		}
		case ActStopTrace:
			StopTrace();
			return LineStatus::Finished;
		default:
			return LineStatus::Unhandled;
	}
}


void TelemetryHost::PrintTelemetry(bool bMarkdown)
{
	std::cout << (bMarkdown?"## ":"") << "Avaliable telemetry streams:\n";
	if (bMarkdown)
	{
		std::cout << "- [By name](#by-name)\n";
		std::cout << "- [By category](#by-category)\n";
		std::cout << "### ";
	}
	else
	{
		std::cout << '\t';
	}
	std::cout << "By Name:\n";
	if (bMarkdown)
	{
		std::cout << "Name | Categories\n";
		std::cout << "-----|-----------\n";
	}
	for (auto &it : m_mCatsByName)
	{
		std::string strCats(bMarkdown?"|":"");
		for (auto &sName : it.second)
		{
			strCats += " `" + m_mCat2Str.at(sName) + "`";
		}
		if (bMarkdown)
		{
			std::cout << it.first << strCats << '\n';
		}
		else
		{
			std::cout << '\t' << std::setw(40) << std::left << it.first << strCats << '\n';
		}

	}
	std::cout << (bMarkdown? "### " : "\t") << "By Category\n";

	for (auto &cat : m_mNamesByCat)
	{
		std::cout << (bMarkdown?"#### ":"\t\t") << m_mCat2Str.at(cat.first) << '\n';
		for (auto &name : cat.second)
		{
			if (bMarkdown)
			{
				std::cout << " - ";
			}
			else
			{
				std::cout << "\t\t\t";
			}
			std::cout << name << '\n';
		}
	}
}
