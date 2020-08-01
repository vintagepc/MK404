/*
	OBJCollection.h - Base class wrangler for a collection of OBJs that comprise a
	set of visuals for a single printer.

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

#include "GLObj.h"
#include <string>
#include <vector>
#include <map>

using namespace std;

class OBJCollection
{
	public:

		OBJCollection(string strName):m_strName(strName){};
		~OBJCollection(){};

		void Load()
		{
			for (auto it = m_mObjs.begin(); it!=m_mObjs.end(); it++)
				for (auto it2 = it->second.begin(); it2!=it->second.end(); it2++)
					(*it2)->Load();
			OnLoadComplete();
		};

		// Note: class reference is for the motion, e.g.
		// an object of class "X" is moved when X changes.
		enum class ObjClass
		{
			X,
			Y,
			Z,
			E,
			Fixed,
			Media,
			PrintSurface,
			Other
		};

		virtual void SetupLighting(){};

		inline void Draw(const ObjClass type)
		{
			if (m_mObjs.count(type)==0)
				return;

			vector<GLObj*> vObj = m_mObjs.at(type);
			for (auto it = vObj.begin(); it!=vObj.end(); it++)
				(*it)->Draw();
		};

		virtual void GetBaseCenter(float fTrans[3])
		{
			m_pBaseObj->GetCenteringTransform(fTrans);
		};

		virtual void OnLoadComplete(){};

		virtual inline void ApplyPLEDTransform() {};
		virtual inline void ApplyLCDTransform() {};
		virtual inline void ApplyPrintTransform(){};

		virtual float GetScaleFactor(){return 1.f;};

		virtual inline void SetNozzleCam(bool bOn) {};

		virtual void GetNozzleCamPos(float fPos[3]) = 0;

		virtual void DrawKnob(int iRotation) = 0;
		virtual void DrawEFan(int iRotation){};
		virtual void DrawPFan(int iRotation){};
		virtual void DrawEVis(float fEPos){};

		virtual bool SupportsMMU() { return false; }

		inline const string GetName() { return m_strName;}

	protected:
		template<typename... Args>GLObj* AddObject(const ObjClass type, Args... args)
		{
			//printf("Added file: %s\n",strFile.c_str());
			auto obj = new GLObj(args...);
			m_mObjs[type].push_back(obj);
			return obj;
		};

		static constexpr float MM_TO_M = 1.f/1000.f;
		static constexpr float CM_TO_M = 1.f/100.f;

		void SetMaterialMode(GLenum type)
		{
			for (auto key = m_mObjs.begin(); key != m_mObjs.end(); key++)
				for (auto it = key->second.begin(); it!=key->second.end(); it++)
					(*it)->SetMaterialMode(type);
		};

		GLObj* m_pBaseObj = nullptr;

		inline void SetName(string strName) { m_strName = strName; }

	private:

		string m_strName; // Collection name.

		map<ObjClass,vector<GLObj*>> m_mObjs = {};

};
