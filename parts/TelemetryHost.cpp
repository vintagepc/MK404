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
#include <iomanip>
#include <iostream>

shared_ptr<TelemetryHost> TelemetryHost::m_pHost;

void TelemetryHost::AddTrace(avr_irq_t *pIRQ, string strName, TelCats vCats, uint8_t uiBits)
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
		cout << "Telemetry: Added trace " << strName << '\n';
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
		cerr << "ERROR: Trying to add the same IRQ "<<  strName <<" to a VCD trace multiple times!\n";
	}
}

void TelemetryHost::SetCategories(const vector<string> &vsCats)
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

Scriptable::LineStatus TelemetryHost::ProcessAction(unsigned int iAct, const vector<string> &vArgs)
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
	cout << (bMarkdown?"## ":"") << "Avaliable telemetry streams:\n";
	if (bMarkdown)
	{
		cout << "- [By name](#by-name)\n";
		cout << "- [By category](#by-category)\n";
		cout << "### ";
	}
	else
	{
		cout << '\t';
	}
	cout << "By Name:\n";
	if (bMarkdown)
	{
		cout << "Name | Categories\n";
		cout << "-----|-----------\n";
	}
	for (auto &it : m_mCatsByName)
	{
		string strCats(bMarkdown?"|":"");
		for (auto &sName : it.second)
		{
			strCats += " `" + m_mCat2Str.at(sName) + "`";
		}
		if (bMarkdown)
		{
			cout << it.first << strCats << '\n';
		}
		else
		{
			cout << '\t' << std::setw(40) << std::left << it.first << strCats << '\n';
		}

	}
	cout << (bMarkdown? "### " : "\t") << "By Category\n";

	for (auto &cat : m_mNamesByCat)
	{
		cout << (bMarkdown?"#### ":"\t\t") << m_mCat2Str.at(cat.first) << '\n';
		for (auto &name : cat.second)
		{
			if (bMarkdown)
			{
				cout << " - ";
			}
			else
			{
				cout << "\t\t\t";
			}
			cout << name << '\n';
		}
	}
}
