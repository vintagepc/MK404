/*
	OBJCollection.h - Base class wrangler for a collection of OBJs that comprise a
	set of visuals for a single printer.

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


#include "OBJCollection.h"
#include "GLObj.h"
#include <map>
#include <memory>

void OBJCollection::Load()
{
	for (auto &sets : m_mObjs)
	{
		for (auto &obj : sets.second)
		{
			obj->Load();
		}
	}
	OnLoadComplete();
};


void OBJCollection::Draw(const ObjClass type)
{
	if (m_mObjs.count(type)==0)
	{
		return;
	}

	for (auto &obj : m_mObjs.at(type))
	{
		obj->Draw();
	}

}


void OBJCollection::SetMaterialMode(GLenum type)
{
	for (auto &sets : m_mObjs)
	{
		for (auto &obj : sets.second)
		{
			obj->SetMaterialMode(type);
		}
	}
};
