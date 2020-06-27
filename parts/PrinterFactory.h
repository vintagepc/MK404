/*
	PrinterFactory.h - Printer factory for printer models.
	Add your printer implementation to the map of names->constructors.
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

#pragma once

#include "Board.h"
#include "Printer.h"
#include "printers/Prusa_MK3SMMU2.h"
#include "printers/Prusa_MK3S.h"


/*
	There's probably a better/cleaner way to do this, but FWIW this is the
	first implementation I put together that worked. It could probably be cleaned up
	a bit by having the printer class be a subclass of Board, but I'm not entirely
	sure that's the right way to go yet. I think time will tell once we get more models
	in place...
*/
//typedef void(*Dtor)();
class PrinterFactory
{
	public:

		template<typename ...Args>
		static void* CreatePrinter(string strPrinter, Boards::Board *&pBoard, Printer *&pPrinter, bool bBL, bool bNoHacks, bool bSerial, string strSD, Args...args)
		{
				void* p = (GetPrinterByName(strPrinter,pBoard,pPrinter));
				if (!strSD.empty()) pBoard->SetSDCardFile(strSD);
				pBoard->SetDisableWorkarounds(bNoHacks);
				pPrinter->SetConnectSerial(bSerial);
				pBoard->CreateBoard(args...);
				if (bBL) pBoard->SetStartBootloader();
				return p;
		};

		static vector<string> GetModels()
		{
			vector<string> strModels;
			for(auto it = m_Models.begin(); it != m_Models.end(); ++it)
				strModels.push_back(it->first);
			return strModels;
		}

		static void* GetPrinterByName(const string &strModel,Boards::Board *&pBoard, Printer *&pPrinter)
		{
			if (!m_Models.count(strModel))
			{
				fprintf(stderr, "ERROR: Cannot create printer model '%s'. It is not registered. (Also, how did you bypass the argument constraints?!?\n",strModel.c_str());
				pBoard = nullptr;
				pPrinter = nullptr;
				return nullptr;
			}
			Ctor fnCreate = m_Models.at(strModel).first;
			return fnCreate(pBoard,pPrinter);
		}

		static void DestroyPrinterByName(const string &strModel, void* p)
		{
			if (!m_Models.count(strModel))
			{
				fprintf(stderr, "ERROR: Cannot delete printer model '%s'. It is not registered. (Also, how did you bypass the argument constraints?!?\n",strModel.c_str());
				return;
			}
			m_Models.at(strModel).second(p);
		}

		template<class P>
		static void* _CreatePrinter(Boards::Board *&pBoard, Printer *&pPrinter)
		{
			P* p = new P();
			pPrinter = p;
			pBoard = p;
			return p;
		};

		template<class P>
		static void _DestroyPrinter(void *p)
		{
			P* printer = (P*)p;
			printer->StopAVR();
			printer->~P();
		};

	private:
		typedef void*(*Ctor)(Boards::Board *&pBoard, Printer *&pPrinter);
		typedef void(*Dtor)(void* p);

		// TODO: maybe a way to have printer classes register dynamically instead of needing to add them to the map?
		static map<string,pair<Ctor,Dtor>> m_Models;
};

map<string,pair<PrinterFactory::Ctor,PrinterFactory::Dtor>>  PrinterFactory::m_Models  = {
	std::make_pair("Prusa_MK3S",		make_pair(&PrinterFactory::_CreatePrinter<Prusa_MK3S>	, &PrinterFactory::_DestroyPrinter<Prusa_MK3S>)),
	std::make_pair("Prusa_MK3SMMU2",	make_pair(&PrinterFactory::_CreatePrinter<Prusa_MK3SMMU2>, &PrinterFactory::_DestroyPrinter<Prusa_MK3SMMU2>))
};
