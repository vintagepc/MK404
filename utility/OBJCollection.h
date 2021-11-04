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
#include "gsl-lite.hpp"  // for span
#include <GL/glew.h>     // for GLenum
#include <map>
#include <memory>
#include <string>
#include <utility>       // for move
#include <vector>

class OBJCollection
{
	public:

		explicit OBJCollection(std::string strName):m_strName(std::move(strName)){};
		~OBJCollection() = default; // pragma: LCOV_EXCL_LINE

		void Load();

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


		void Draw(const ObjClass type);

		virtual inline void ApplyPLEDTransform() {}; // pragma: LCOV_EXCL_START  - these lines are not reachable if derivatives are properly implemented.

		virtual inline void GetBaseCenter(gsl::span<float> fTrans)
		{
			m_pBaseObj->GetCenteringTransform(fTrans);
		};

		virtual void OnLoadComplete(){};
		virtual void SetupLighting(){};
		virtual inline void ApplyBedLEDTransform() {};
		virtual inline void ApplyLCDTransform() {};
		virtual inline void ApplyPrintTransform(){};

		virtual inline float GetScaleFactor(){return 1.f;};

		virtual inline void SetNozzleCam(bool /*bOn*/) {};

		virtual void GetNozzleCamPos(gsl::span<float> fPos) = 0;

		virtual void DrawKnob(int iRotation) = 0;
		virtual void DrawEFan(int /*iRotation*/){};
		virtual void DrawPFan(int /*iRotation*/){};
		virtual void DrawEVis(float /*fEPos*/){};

		virtual void DrawGeneric1 (uint32_t uiVal) {};
		virtual void DrawGeneric2 (uint32_t uiVal) {};
		virtual void DrawGeneric3 (uint32_t uiVal) {};

		// pragma: LCOV_EXCL_STOP
		virtual inline bool SupportsMMU() { return false; }


		inline const std::string GetName() { return m_strName;}

	protected:
		template<typename... Args>std::shared_ptr<GLObj> AddObject(const ObjClass type, Args... args)
		{
			//printf("Added file: %s\n",strFile.c_str());
			std::shared_ptr<GLObj> obj(new GLObj(args...));
			m_mObjs[type].push_back(obj);
			return obj;
		};

		static constexpr float MM_TO_M = 1.f/1000.f;
		static constexpr float CM_TO_M = 1.f/100.f;

		void SetMaterialMode(GLenum type);

		std::shared_ptr<GLObj> m_pBaseObj = nullptr;

		inline void SetName(std::string strName) { m_strName = std::move(strName); }
		std::map<ObjClass,std::vector<std::shared_ptr<GLObj>>> m_mObjs = {};

	private:

		std::string m_strName; // Collection name.


};
