/*
	GLObj.h - a GL wrangler class for .obj files/visuals.

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


#include <vector>
#include <type_traits>
#include <tiny_obj_loader.h>
#include <GL/glew.h>

class GLObj 
{
    public:
        // Creates a new GLObj for the given .obj file
        GLObj(std::string strFile);

        // Loads the file. This allows you to have fixed members but delay load them if needed.
        void Load();

        // Draws the .obj within the current GL matrix transformation. Does nothing if !loaded.
        void Draw();

        // Lets you show or hide a specific sub-object within the .obj.
        void SetSubobjectVisible(uint iObj, bool bVisible = true);

        // Lets you tweak the material for a subobject. An example is a modelled LED turning on/off.
        void SetSubobjectMaterial(uint iObj, int iMat);

        // Sets all subobjects as visible or invisible.
        void SetAllVisible(bool bVisible = true);

        // Returns the biggest dimension's size (for scaling to unit size)
        float GetScaleFactor() { return m_fMaxExtent; };

        // Gets the transform float you need to center the obj at 0,0,0.
        void GetCenteringTransform(float fTrans[3]) { for (int i=0; i<3; i++) fTrans[i] = -0.5f * (m_extMin[i] + m_extMax[i]);}


    private:

        typedef struct {
            GLuint vb;  // vertex buffer
            int numTriangles;
            size_t material_id;
            bool bDraw;
        } DrawObject;
        float m_fMaxExtent;
        float m_extMin[3], m_extMax[3];
        bool m_bLoaded = false;
        std::vector<tinyobj::material_t> m_materials;
        std::map<std::string, GLuint> m_textures;        
        std::vector<DrawObject> m_DrawObjects;
        std::string m_strFile;

        // Load helper from the tinyobjloader example.
        bool LoadObjAndConvert(const char* filename);
};
