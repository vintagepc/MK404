/*
	KeyController.h - Wrangler for dispatching keypresses to KeyClients.

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

#include "IScriptable.h"
#include "Scriptable.h"
#include <atomic>
#include <map>               // for map
#include <string>            // for string
#include <vector>            // for vector

static constexpr uint8_t SPECIAL_KEY_MASK = 0x80;

class IKeyClient;

class KeyController: private Scriptable
{
	friend IKeyClient;

	public:
		static KeyController& GetController();

		// Called by the key handler to notify a key was pressed.
		inline void OnKeyPressed(unsigned char key) { m_key.store(key); };

		// Called by the printer so key events happen "safely" on AVR cycles.
		void OnAVRCycle();

		void PrintKeys(bool bMarkdown);

		static inline void GLKeyReceiver(unsigned char key, int /*x*/, int /*y*/) { KeyController::GetController().OnKeyPressed(key); };
		static inline void GLSpecialKeyReceiver(int key, int /*x*/, int /*y*/) { KeyController::GetController().OnKeyPressed(key | SPECIAL_KEY_MASK); };

	protected:
		KeyController();
		~KeyController() override = default;

		// Invoked by IKeyClient to add a client.
		void AddKeyClient(IKeyClient *pClient, const unsigned char key, const std::string &strDesc);

		LineStatus ProcessAction(unsigned int iAction, const std::vector<std::string> &args) override;

	private:
		void PutNiceKeyName(unsigned char key);

		std::map<unsigned char, std::vector<IKeyClient*>> m_mClients {};
		std::map<unsigned char, std::string> m_mDescrs {};
		std::atomic_uchar m_key {0};

};
