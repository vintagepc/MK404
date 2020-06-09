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
template<class P>
class _PrinterFactory
{
	public:
		static void CreatePrinter(Boards::Board *&pBoard, Printer *&pPrinter)
		{
			P* p = new P();
			pPrinter = p;
			pBoard = p;
		};
};

class PrinterFactory
{
	public:
		template<typename ...Args>
		static void CreatePrinter(string strPrinter, Boards::Board *&pBoard, Printer *&pPrinter, bool bBL, bool bNoHacks, Args...args)
		{
				GetPrinterByName(strPrinter,pBoard,pPrinter);
				if (bBL) pBoard->SetStartBootloader();
				pBoard->SetDisableWorkarounds(bNoHacks);
				pBoard->CreateBoard(args...);
		};

		static vector<string> GetModels()
		{
			auto models = GetMap();
			vector<string> strModels;
			for(map<string,Ctor>::iterator it = models.begin(); it != models.end(); ++it)
				strModels.push_back(it->first);
			return strModels;
		}

		static void GetPrinterByName(const string &strModel,Boards::Board *&pBoard, Printer *&pPrinter)
		{
			auto models = GetMap();
			if (!models.count(strModel))
			{
				fprintf(stderr, "ERROR: Cannot create printer model '%s'. It is not registered. (Also, how did you bypass the argument constraints?!?\n",strModel.c_str());
				pBoard = nullptr;
				pPrinter = nullptr;
			}
			models.at(strModel)(pBoard,pPrinter);

		}

	private:
		typedef void(*Ctor)(Boards::Board *&pBoard, Printer *&pPrinter);
		// TODO: maybe a way to have printer classes register dynamically instead of needing to add them to the map?
		static map<string,Ctor> GetMap() {
			map<string,Ctor> m_Models = {
				std::make_pair("Prusa_MK3S",&_PrinterFactory<Prusa_MK3S>::CreatePrinter),
				std::make_pair("Prusa_MK3SMMU2",&_PrinterFactory<Prusa_MK3SMMU2>::CreatePrinter),
			};
			return m_Models;
		};
};
