/*
	KeyController.cpp - Wrangler for dispatching keypresses to KeyClients.

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

#include "KeyController.h"
#include "IKeyClient.h"
#include <iostream>
#include <string>
#include <utility>
#include <vector>

KeyController& KeyController::GetController()
{
	static KeyController k {};
	return k;
}


void KeyController::AddKeyClient(IKeyClient *pClient, const unsigned char key, const std::string &strDesc)
{
	m_mClients[key].push_back(pClient);
	if (!strDesc.empty())
	{
		m_mDescrs[key].append(strDesc + " ");
	}
}

void KeyController::PrintKeys(bool /*bMarkdown*/)
{
	std::cout << "Available Key Controls:\n";
	for (auto it : m_mDescrs)
	{
		std::cout << '\t';
		PutNiceKeyName(it.first);
		std::cout << ":\t" << it.second << '\n';
	}

}

void KeyController::PutNiceKeyName(unsigned char key)
{
	switch(key)
	{
		case 0xd:
			std::cout << "Enter";
			break;
		default:
			std::cout << key;
			break;
	}
}

void KeyController::OnAVRCycle()
{
	auto key = m_key.load();

	if (key==0)
	{
		return;
	}

	m_key.store(0);

	std::vector<IKeyClient*>& vClients = m_mClients.at(key);
	for (auto c: vClients)
	{
		c->OnKeyPress(key);
	}

}
