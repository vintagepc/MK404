/*
	PrinterFactory.cpp - Printer factory for printer models.
	Add your printer implementation to the map of names->constructors.
	Copyright 2020-2021 VintagePC <https://github.com/vintagepc/>

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

#include "PrinterFactory.h"
#include "printers/Prusa_CW1.h"
#include "printers/Prusa_CW1S.h"
#include "printers/Prusa_MK1_13.h"
#include "printers/Prusa_MK25S_13.h"
#include "printers/Prusa_MK25_13.h"
#include "printers/Prusa_MK2MMU_13.h"
#include "printers/Prusa_MK2_13.h"
#include "printers/Prusa_MK3.h"
#include "printers/Prusa_MK3MMU2.h"
#include "printers/Prusa_MK3S.h"
#include "printers/Prusa_MK3SMMU2.h"
#include "printers/Prusa_MMU2.h"
#include "printers/Test_Printer.h"
#include "printers/IPCPrinter.h"
#include "printers/IPCPrinter_MMU2.h"
#include <algorithm>                   // for max
#include <iostream>

/*
	There's probably a better/cleaner way to do this, but FWIW this is the
	first implementation I put together that worked. It could probably be cleaned up
	a bit by having the printer class be a subclass of Board, but I'm not entirely
	sure that's the right way to go yet. I think time will tell once we get more models
	in place...
*/


std::vector<std::string> PrinterFactory::GetModels()
{
	std::vector<std::string> strModels;
	for(auto &models: GetModelMap())
	{
		strModels.push_back(models.first);
	}
	return strModels;
}

void* PrinterFactory::GetPrinterByName(const std::string &strModel,Boards::Board *&pBoard, Printer *&pPrinter) //NOLINT
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

void PrinterFactory::DestroyPrinterByName(const std::string &strModel, void* p)
{
	if (!GetModelMap().count(strModel))
	{
		std::cerr << "ERROR: Cannot delete printer model, It is not registered. (Also, how did you bypass the argument constraints?!?\n";
		return;
	}
	GetModelMap().at(strModel).second(p);
}

using Ctor = void*(*)(Boards::Board *&pBoard, Printer *&pPrinter);
using Dtor = void(*)(void* p);

// Someday: maybe a way to have printer classes register dynamically instead of needing to add them to the map?
std::map<std::string,std::pair<Ctor,Dtor>>& PrinterFactory::GetModelMap()
{
	static std::map<std::string,std::pair<PrinterFactory::Ctor,PrinterFactory::Dtor>> m_Models  = {
		{"Prusa_CW1", 			{&PrinterFactory::_CreatePrinter<Prusa_CW1>	, 	&PrinterFactory::_DestroyPrinter<Prusa_CW1>}},
		{"Prusa_CW1S", 			{&PrinterFactory::_CreatePrinter<Prusa_CW1S>	, 	&PrinterFactory::_DestroyPrinter<Prusa_CW1S>}},
		{"Prusa_MK1_mR13",		{&PrinterFactory::_CreatePrinter<Prusa_MK1_13>	, 	&PrinterFactory::_DestroyPrinter<Prusa_MK1_13>}},
		{"Prusa_MK2_mR13",		{&PrinterFactory::_CreatePrinter<Prusa_MK2_13>	, 	&PrinterFactory::_DestroyPrinter<Prusa_MK2_13>}},
		{"Prusa_MK2MMU_mR13",	{&PrinterFactory::_CreatePrinter<Prusa_MK2MMU_13>, 	&PrinterFactory::_DestroyPrinter<Prusa_MK2MMU_13>}},
		{"Prusa_MK25_mR13",		{&PrinterFactory::_CreatePrinter<Prusa_MK25_13>	, 	&PrinterFactory::_DestroyPrinter<Prusa_MK25_13>}},
		{"Prusa_MK25S_mR13",	{&PrinterFactory::_CreatePrinter<Prusa_MK25S_13>, 	&PrinterFactory::_DestroyPrinter<Prusa_MK25S_13>}},
		{"Prusa_MK3",			{&PrinterFactory::_CreatePrinter<Prusa_MK3>	, 		&PrinterFactory::_DestroyPrinter<Prusa_MK3>}},
		{"Prusa_MK3S",			{&PrinterFactory::_CreatePrinter<Prusa_MK3S>	, 	&PrinterFactory::_DestroyPrinter<Prusa_MK3S>}},
		{"Prusa_MK3SMMU2",		{&PrinterFactory::_CreatePrinter<Prusa_MK3SMMU2>, 	&PrinterFactory::_DestroyPrinter<Prusa_MK3SMMU2>}},
		{"Prusa_MK3MMU2",		{&PrinterFactory::_CreatePrinter<Prusa_MK3MMU2>, 	&PrinterFactory::_DestroyPrinter<Prusa_MK3MMU2>}},
		{"Prusa_MMU2",			{&PrinterFactory::_CreatePrinter<Prusa_MMU2>, 		&PrinterFactory::_DestroyPrinter<Prusa_MMU2>}},
		{"IPCPrinter",			{&PrinterFactory::_CreatePrinter<IPCPrinter>, 		&PrinterFactory::_DestroyPrinter<IPCPrinter>}},
		{"IPCPrinter_MMU2",		{&PrinterFactory::_CreatePrinter<IPCPrinter_MMU2>, 	&PrinterFactory::_DestroyPrinter<IPCPrinter_MMU2>}},
		{"Test_Printer",		{&PrinterFactory::_CreatePrinter<Test_Printer>, 	&PrinterFactory::_DestroyPrinter<Test_Printer>}}
	};
	return m_Models;
}
