/*
	GLObj.h - a GL wrangler class for .obj files/visuals.

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

#include "tiny_obj_loader.h"  // for material_t
#include "gsl-lite.hpp"
#include <GL/glew.h>          // for GLuint
#include <map>                // for map
#include <stddef.h>           // for size_t
#include <string>             // for string
#include <vector>             // for vector
#include <mutex>

using namespace std;

class GLObj
{
    public:
        // Creates a new GLObj for the given .obj file
		GLObj(std::string strFile);
		GLObj(std::string strFile, float fScale);
        GLObj(std::string strFile, float fTX, float fTY, float fTZ,  float fScale = 1.f);

		enum class SwapMode
		{
			NONE,
			YMINUSZ
		};

        // Loads the file. This allows you to have fixed members but delay load them if needed.
        void Load();

        // Draws the .obj within the current GL matrix transformation. Does nothing if !loaded.
        void Draw();

		// Swaps axes on load .
		inline void SetSwapMode(SwapMode mode){m_swapMode = mode;}

		// Sets material type for mtls with fewer parameters.
		inline void SetMaterialMode(GLenum mode){m_matMode = mode;}

        // Lets you show or hide a specific sub-object within the .obj.
        void SetSubobjectVisible(unsigned iObj, bool bVisible = true);

        // Lets you tweak the material for a subobject. An example is a modelled LED turning on/off.
        void SetSubobjectMaterial(unsigned iObj, unsigned iMat);

        // Sets all subobjects as visible or invisible.
        void SetAllVisible(bool bVisible = true);

		// Prevents recalculation of normals on scaling.
		void SetKeepNormalsIfScaling(bool bKeep) { m_bNoNewNormals = bKeep;}

        // Returns the biggest dimension's size (for scaling to unit size)
        float GetScaleFactor() { return m_fMaxExtent; };

        // Gets the transform float you need to center the obj at 0,0,0.
        void GetCenteringTransform(float fTrans[3]) { for (int i=0; i<3; i++) fTrans[i] = -0.5f * ((m_extMin[i] + m_extMax[i]));}


    private:

        typedef struct {
            GLuint vb;  // vertex buffer
            int numTriangles;
            size_t material_id; // Atomic to allow for cross thread
            bool bDraw;
        } DrawObject;
        float m_fMaxExtent = 1.f;
        float _m_extMin[3] = {0,0,0}, _m_extMax[3] = {0,0,0};
		gsl::span<float> m_extMin {_m_extMin}, m_extMax {_m_extMax};
        bool m_bLoaded = false, m_bNoNewNormals = false;
        vector<tinyobj::material_t> m_materials;
        map<std::string, GLuint> m_textures;
        vector<DrawObject> m_DrawObjects;

		GLenum m_matMode = GL_DIFFUSE;

        string m_strFile;
		float m_fScale = 1.f;
		float m_fCorr[3] = {0,0,0};

		SwapMode m_swapMode = SwapMode::NONE;

		mutex m_lock;

        // Load helper from the tinyobjloader example.
        bool LoadObjAndConvert(const char* filename);

		void AddObject(const vector<float> &vb, int iMatlId);
};
