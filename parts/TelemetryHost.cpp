/*
	TelemetryHost.cpp - Wrangler for VCD traces and scripted telemetry

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

#include "TelemetryHost.h"
#include <algorithm>


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
		m_mIRQs[strName] = pIRQ;
	else
		fprintf(stderr, "ERROR: Trying to add the same IRQ (%s) to a VCD trace multiple times!\n",strName.c_str());
}

void TelemetryHost::SetCategories(const vector<string> &vsCats)
{
	for (auto it = vsCats.begin(); it!=vsCats.end(); it++)
	{
		if (!m_mCats.count(it[0]))
			m_vsNames.push_back(it[0]); // Save non-category for name check later.
		else if (find(m_VLoglst.begin(), m_VLoglst.end(), m_mCats.at(it[0]))==m_VLoglst.end())
			m_VLoglst.push_back(m_mCats.at(it[0]));
	}
}

Scriptable::LineStatus TelemetryHost::ProcessAction(unsigned int iAct, const vector<string> &vArgs)
{
	switch (iAct)
	{
		case ActWaitFor:
		{
			if (m_pCurrentIRQ == nullptr)
			{
				if (!m_mIRQs.count(vArgs.at(0)))
					return IssueLineError("Asked to wait for telemetry " + vArgs.at(0) + " but it was not found");
				else
				{
					m_pCurrentIRQ = m_mIRQs[vArgs.at(0)];
					m_uiMatchVal = stoul(vArgs.at(1));
				}
			}
			if (m_pCurrentIRQ->value == m_uiMatchVal)
			{
				m_pCurrentIRQ = nullptr;
				return LineStatus::Finished;
			}
			else
				return LineStatus::Waiting;
		}
		default:
			return LineStatus::Unhandled;
	}
}


void TelemetryHost::PrintTelemetry()
{
	printf("Avaliable telemetry in the current context:\n");
	for (auto it = m_mIRQs.begin(); it!=m_mIRQs.end(); it++)
		printf("\t%s\n",it->first.c_str());
}
