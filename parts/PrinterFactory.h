/*
	PrinterFactory.h - Printer factory for printer models.
	Add your printer implementation to the map of names->constructors.
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

#include "Board.h"
#include "Printer.h"
#include "printers/Prusa_MK3.h"
#include "printers/Prusa_MK3MMU2.h"
#include "printers/Prusa_MK3S.h"
#include "printers/Prusa_MK3SMMU2.h"
#ifdef TEST_PRINTER
#include "printers/Test_Printer.h"
#endif // TEST_PRINTER
#include <iostream>

/*
	There's probably a better/cleaner way to do this, but FWIW this is the
	first implementation I put together that worked. It could probably be cleaned up
	a bit by having the printer class be a subclass of Board, but I'm not entirely
	sure that's the right way to go yet. I think time will tell once we get more models
	in place...
*/
class PrinterFactory
{
	public:

		template<typename ...Args>
		static void* CreatePrinter(const std::string &strPrinter, Boards::Board *&pBoard, Printer *&pPrinter, bool bBL, bool bNoHacks, bool bSerial, const std::string &strSD, Args...args)//NOLINT
		{
				void* p = (GetPrinterByName(strPrinter,pBoard,pPrinter));
				if(p != nullptr)
				{
					if (!strSD.empty()) pBoard->SetSDCardFile(strSD);
					pBoard->SetDisableWorkarounds(bNoHacks);
					pPrinter->SetConnectSerial(bSerial);
					pBoard->CreateBoard(args...);
					if (bBL) pBoard->SetStartBootloader();
				}
				return p;
		};

		static std::vector<std::string> GetModels()
		{
			std::vector<std::string> strModels;
			for(auto &models: GetModelMap())
			{
				strModels.push_back(models.first);
			}
			return strModels;
		}

		static void* GetPrinterByName(const std::string &strModel,Boards::Board *&pBoard, Printer *&pPrinter) //NOLINT
		{
			if (!GetModelMap().count(strModel))
			{
				std::cerr << "ERROR: Cannot create printer model. It is not registered. (Also, how did you bypass the argument constraints?!?\n";
				pBoard = nullptr;
				pPrinter = nullptr;
				return nullptr;
			}
			Ctor fnCreate = GetModelMap().at(strModel).first;
			return fnCreate(pBoard,pPrinter);
		}

		static void DestroyPrinterByName(const std::string &strModel, void* p)
		{
			if (!GetModelMap().count(strModel))
			{
				std::cerr << "ERROR: Cannot delete printer model, It is not registered. (Also, how did you bypass the argument constraints?!?\n";
				return;
			}
			GetModelMap().at(strModel).second(p);
		}

		template<class P>
		static void* _CreatePrinter(Boards::Board *&pBoard, Printer *&pPrinter)//NOLINT
		{
			auto p = new P(); //NOLINT
			pPrinter = p;
			pBoard = p;
			return p;
		};

		template<class P>
		static void _DestroyPrinter(void *p)
		{
			auto printer = static_cast<P*>(p);
			printer->StopAVR();
			printer->~P();
		};

	private:
		using Ctor = void*(*)(Boards::Board *&pBoard, Printer *&pPrinter);
		using Dtor = void(*)(void* p);

		// Someday: maybe a way to have printer classes register dynamically instead of needing to add them to the map?
		static std::map<std::string,std::pair<Ctor,Dtor>>& GetModelMap()
		{
			static std::map<std::string,std::pair<PrinterFactory::Ctor,PrinterFactory::Dtor>> m_Models  = {
				{"Prusa_MK3",			{&PrinterFactory::_CreatePrinter<Prusa_MK3>	, 		&PrinterFactory::_DestroyPrinter<Prusa_MK3>}},
				{"Prusa_MK3S",			{&PrinterFactory::_CreatePrinter<Prusa_MK3S>	, 	&PrinterFactory::_DestroyPrinter<Prusa_MK3S>}},
				{"Prusa_MK3SMMU2",		{&PrinterFactory::_CreatePrinter<Prusa_MK3SMMU2>, 	&PrinterFactory::_DestroyPrinter<Prusa_MK3SMMU2>}},
#ifdef TEST_PRINTER // png++ doesn't work with apple
				{"Test_Printer",		{&PrinterFactory::_CreatePrinter<Test_Printer>, 	&PrinterFactory::_DestroyPrinter<Test_Printer>}},
#endif // TEST_PRINTER
				{"Prusa_MK3MMU2",		{&PrinterFactory::_CreatePrinter<Prusa_MK3MMU2>, 	&PrinterFactory::_DestroyPrinter<Prusa_MK3MMU2>}}
			};
			return m_Models;
		}
};
