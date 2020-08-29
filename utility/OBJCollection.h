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

#pragma once

#include "GLObj.h"
#include <map>
#include <string>
#include <vector>

class OBJCollection
{
	public:

		explicit OBJCollection(std::string strName):m_strName(std::move(strName)){};
		~OBJCollection() = default;

		void Load()
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
			{
				return;
			}

			std::vector<GLObj*> vObj = m_mObjs.at(type);
			for (auto &obj : vObj)
			{
				obj->Draw();
			}

		};

		virtual void GetBaseCenter(gsl::span<float> fTrans)
		{
			m_pBaseObj->GetCenteringTransform(fTrans);
		};

		virtual void OnLoadComplete(){};

		virtual inline void ApplyPLEDTransform() {};
		virtual inline void ApplyBedLEDTransform() {};
		virtual inline void ApplyLCDTransform() {};
		virtual inline void ApplyPrintTransform(){};

		virtual float GetScaleFactor(){return 1.f;};

		virtual inline void SetNozzleCam(bool /*bOn*/) {};

		virtual void GetNozzleCamPos(gsl::span<float> fPos) = 0;

		virtual void DrawKnob(int iRotation) = 0;
		virtual void DrawEFan(int /*iRotation*/){};
		virtual void DrawPFan(int /*iRotation*/){};
		virtual void DrawEVis(float /*fEPos*/){};

		virtual bool SupportsMMU() { return false; }

		inline const std::string GetName() { return m_strName;}

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
			for (auto &sets : m_mObjs)
			{
				for (auto &obj : sets.second)
				{
					obj->SetMaterialMode(type);
				}
			}
		};

		GLObj* m_pBaseObj = nullptr;

		inline void SetName(std::string strName) { m_strName = std::move(strName); }
		std::map<ObjClass,std::vector<GLObj*>> m_mObjs = {};

	private:

		std::string m_strName; // Collection name.


};
