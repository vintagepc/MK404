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
#include <algorithm>       // for find
#include "sim_vcd_file.h"  // for avr_vcd_add_signal


TelemetryHost* TelemetryHost::m_pHost = new TelemetryHost();

void TelemetryHost::AddTrace(avr_irq_t *pIRQ, string strName, TelCats vCats, uint8_t uiBits)
{
	bool bShouldAdd = false;
	// Check categories.
	for (auto it = vCats.begin(); it!=vCats.end(); it++)
		if (find(m_VLoglst.begin(), m_VLoglst.end(), it[0])!=m_VLoglst.end())
		{
			bShouldAdd = true;
			break;
		}

	strName+= "_";
	strName.append(pIRQ->name);
	// Check explicit names
	for (auto it = m_vsNames.begin(); it!=m_vsNames.end(); it++)
		if (strName.rfind(it[0],0)==0)
		{
			bShouldAdd = true;
			break;
		}


	if (bShouldAdd)
	{
		printf("Telemetry: Added trace %s\n",strName.c_str());
		avr_vcd_add_signal(&m_trace, pIRQ, uiBits, strName.c_str());
	}
	if (!m_mIRQs.count(strName))
	{
		m_mIRQs[strName] = pIRQ;
		m_mCatsByName[strName] = vCats;
		for(auto it = vCats.begin(); it!=vCats.end(); it++)
			m_mNamesByCat[*it].push_back(strName);
	}
	else
		fprintf(stderr, "ERROR: Trying to add the same IRQ (%s) to a VCD trace multiple times!\n",strName.c_str());
}

void TelemetryHost::SetCategories(const vector<string> &vsCats)
{
	for (auto it = vsCats.begin(); it!=vsCats.end(); it++)
	{
		if (!m_mStr2Cat.count(it[0]))
			m_vsNames.push_back(it[0]); // Save non-category for name check later.
		else if (find(m_VLoglst.begin(), m_VLoglst.end(), m_mStr2Cat.at(it[0]))==m_VLoglst.end())
			m_VLoglst.push_back(m_mStr2Cat.at(it[0]));
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
					return IssueLineError("Asked to wait for telemetry " + vArgs.at(0) + " but it was not found");
				else
				{
					m_pCurrentIRQ = m_mIRQs[vArgs.at(0)];
					uint32_t tmpval = stoul(vArgs.at(1));
					if (tmpval!=m_uiMatchVal)
						printf("WF: %08x (%u) //  %08x (%u)\n",tmpval,tmpval,m_pCurrentIRQ->value, m_pCurrentIRQ->value);
					m_uiMatchVal = stoul(vArgs.at(1));

				}
			}
			bool bMatch = false;
			if (iAct==ActWaitForLT)
				bMatch = m_pCurrentIRQ->value < m_uiMatchVal;
			else if (iAct == ActWaitForGT)
				bMatch = m_pCurrentIRQ->value > m_uiMatchVal;
			else
				bMatch = m_pCurrentIRQ->value == m_uiMatchVal;

			if (bMatch)
			{
				m_pCurrentIRQ = nullptr;
				return LineStatus::Finished;
			}
			else
				return LineStatus::Waiting;
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
	printf("%sAvaliable telemetry streams:\n",bMarkdown?"## ":"");
	if (bMarkdown)
	{
		printf("- [By name](#by-name)\n");
		printf("- [By category](#by-category)\n");
	}
	printf("%sBy Name:\n", bMarkdown?"### ":"\t");
	if (bMarkdown)
	{
		printf("Name | Categories\n");
		printf("-----|-----------\n");
	}
	for (auto it = m_mCatsByName.begin(); it!=m_mCatsByName.end(); it++)
	{
		string strCats(bMarkdown?"|":"");
		for (auto it2 = it->second.begin(); it2!=it->second.end(); it2++)
			strCats += " `" + m_mCat2Str.at(*it2) + "`";

		if (bMarkdown)
			printf("%s%s\n",it->first.c_str(),strCats.c_str());
		else
			printf("\t%-40s%s\n",it->first.c_str(),strCats.c_str());
	}
	printf("%sBy category\n",bMarkdown?"### ":"\t");
	for (auto it = m_mNamesByCat.begin(); it!=m_mNamesByCat.end(); it++)
	{
		printf("%s%s\n",bMarkdown?"#### ":"\t\t",m_mCat2Str.at(it->first).c_str());
		for (auto it2 = it->second.begin(); it2!=it->second.end(); it2++)
			printf("%s%s\n", bMarkdown?" - ":"\t\t\t",it2->c_str());
	}
}
