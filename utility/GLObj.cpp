/*
	GLObj.cpp - a GL wrangler class for .obj files/visuals.

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


#include "GLObj.h"
#include <GL/glew.h>          // for glMaterialfv, GL_FRONT, glBindTexture
#include <algorithm>          // for max, min
#include <cassert>            // for assert
#include <cmath>              // for sqrtf
#include <cstdio>             // for printf, size_t
#include <cstring>            // for memcpy
#include <iostream>           // for operator<<, endl, basic_ostream, cerr
#include <limits>             // for numeric_limits
#include <map>                // for map, _Rb_tree_iterator
#include <scoped_allocator>   // for allocator_traits<>::value_type
#include <string>             // for string, operator<<, char_traits
#include <vector>             // for vector
#include "tiny_obj_loader.h"  // for attrib_t, index_t, mesh_t, shape_t, Loa...


// This disables textures and vertex colors, not used in favor of materials anyway.
// Also cuts GPU RAM usage in half, and probably has performance gains for not needing to set the vertex properties.
#define TEX_VCOLOR 0

GLObj::GLObj(const std::string &strFile,  float fTX, float fTY, float fTZ, float fScale):m_strFile(strFile),m_fScale(fScale),m_fCorr{fTX,fTY,fTZ}
{
}

GLObj::GLObj(const std::string &strFile, float fScale):m_strFile(strFile),m_fScale(fScale)
{
}
GLObj::GLObj(const std::string &strFile):m_strFile(strFile)
{
}


void GLObj::Load()
{
	m_bLoaded = LoadObjAndConvert(m_strFile.c_str());
	if (!m_bLoaded)
		printf("Failed to load obj\n");

	m_fMaxExtent = 0.5f * (m_extMax[0] - m_extMin[0]);
	if (m_fMaxExtent < 0.5f * (m_extMax[1] - m_extMin[1])) {
			m_fMaxExtent = 0.5f * (m_extMax[1] - m_extMin[1]);
	}
	if (m_fMaxExtent < 0.5f * (m_extMax[2] - m_extMin[2])) {
			m_fMaxExtent = 0.5f * (m_extMax[2] - m_extMin[2]);
	}
}

void GLObj::SetAllVisible(bool bVisible)
{
	lock_guard<mutex> lock(m_lock);
	for (size_t i=0; i<m_DrawObjects.size(); i++)
		m_DrawObjects[i].bDraw = bVisible;
}

void GLObj::SetSubobjectVisible(uint iObj, bool bVisible)
{
	lock_guard<mutex> lock(m_lock);
	if (iObj<m_DrawObjects.size())
		m_DrawObjects[iObj].bDraw = bVisible;
}


void GLObj::SetSubobjectMaterial(uint iObj, uint iMat)
{
	if (iObj<m_DrawObjects.size() && iMat < m_materials.size())
	{
		lock_guard<mutex> lock(m_lock);
		//printf("Changed obj %d to %d\n",iObj,iMat);
		m_DrawObjects[iObj].material_id = iMat;
	}
	else
		printf("GLObj: Tried to set invalid material or object.\n");
}

void GLObj::Draw() {
	if (!m_bLoaded)
		return;

	glPolygonMode(GL_FRONT, GL_FILL);
	glPolygonMode(GL_BACK, GL_FILL);

#if TEX_VCOLOR
	GLsizei stride = (3 + 3 + 3 + 2) * sizeof(float);
#else
	GLsizei stride = (3 + 3) * sizeof(float);
#endif
	glPushMatrix();
	glTranslatef(m_fCorr[0],m_fCorr[1],m_fCorr[2]);
	//glScalef(m_fScale,m_fScale,m_fScale);
	if (m_swapMode == SwapMode::YMINUSZ)
		glRotatef(-90,1,0,0);
	lock_guard<mutex> lock(m_lock);
	for (size_t i = 0; i < m_DrawObjects.size(); i++) {
		DrawObject o = m_DrawObjects.at(i);
		if (o.vb < 1 || !o.bDraw) {
			continue;
		}
		glBindBuffer(GL_ARRAY_BUFFER, o.vb);
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_NORMAL_ARRAY);

#if TEX_VCOLOR
		glEnableClientState(GL_COLOR_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
#endif

		if ((o.material_id < m_materials.size())) {
			std::string diffuse_texname = m_materials[o.material_id].diffuse_texname;
			if (m_textures.find(diffuse_texname) != m_textures.end()) {
				glBindTexture(GL_TEXTURE_2D, m_textures[diffuse_texname]);
			} else {
				float fCopy[4] = {0,0,0,1.0f};
				memcpy(fCopy,m_materials[o.material_id].ambient,3*(sizeof(float)));
				glMaterialfv(GL_FRONT, GL_AMBIENT,  fCopy);
				memcpy(fCopy,m_materials[o.material_id].diffuse,3*(sizeof(float)));
				glMaterialfv(GL_FRONT, m_matMode, fCopy);
				memcpy(fCopy,m_materials[o.material_id].specular,3*(sizeof(float)));
				glMaterialfv(GL_FRONT, GL_SPECULAR, fCopy);
				glMaterialf(GL_FRONT, GL_SHININESS, (m_materials[o.material_id].shininess/1000.f)*128.f);
				memcpy(fCopy,m_materials[o.material_id].emission,3*(sizeof(float)));
				glMaterialfv(GL_FRONT, GL_EMISSION, fCopy);
			}

		}
		glVertexPointer(3, GL_FLOAT, stride, (const void*)0);
		glNormalPointer(GL_FLOAT, stride, (const void*)(sizeof(float) * 3));
#if TEX_VCOLOR
		glColorPointer(3, GL_FLOAT, stride, (const void*)(sizeof(float) * 6));
		glTexCoordPointer(2, GL_FLOAT, stride, (const void*)(sizeof(float) * 9));
#endif
		glDrawArrays(GL_TRIANGLES, 0, 3 * o.numTriangles);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindBuffer(GL_ARRAY_BUFFER,0);
	}
	glPopMatrix();
}


static std::string GetBaseDir(const std::string &filepath) {
	if (filepath.find_last_of("/\\") != std::string::npos)
		return filepath.substr(0, filepath.find_last_of("/\\"));
	return "";
}

static void CalcNormal(float N[3], float v0[3], float v1[3], float v2[3]) {
	float v10[3];
	v10[0] = v1[0] - v0[0];
	v10[1] = v1[1] - v0[1];
	v10[2] = v1[2] - v0[2];

	float v20[3];
	v20[0] = v2[0] - v0[0];
	v20[1] = v2[1] - v0[1];
	v20[2] = v2[2] - v0[2];

	N[0] = v20[1] * v10[2] - v20[2] * v10[1];
	N[1] = v20[2] * v10[0] - v20[0] * v10[2];
	N[2] = v20[0] * v10[1] - v20[1] * v10[0];

	float len2 = N[0] * N[0] + N[1] * N[1] + N[2] * N[2];
	if (len2 > 0.0f) {
		float len = sqrtf(len2);

		N[0] /= len;
		N[1] /= len;
	}
}

bool GLObj::LoadObjAndConvert(const char* filename) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;

	std::string base_dir = GetBaseDir(filename);
#ifdef _WIN32
	base_dir += "\\";
#else
	base_dir += "/";
#endif

	std::string err, warn;
	bool ret =
			tinyobj::LoadObj(&attrib, &shapes, &m_materials, &err, filename, base_dir.c_str());
	if (!err.empty()) {
		std::cerr << err << std::endl;
	}

	if (!ret) {
		std::cerr << "Failed to load " << filename << std::endl;
		return false;
	}
	for (size_t i = 0; i<attrib.vertices.size(); i++)
		attrib.vertices[i] *= m_fScale;

	printf("##### %s #####\n",filename);
	printf("# of vertices  = %d\n", (int)(attrib.vertices.size()) / 3);
	printf("# of normals   = %d\n", (int)(attrib.normals.size()) / 3);
	printf("# of texcoords = %d\n", (int)(attrib.texcoords.size()) / 2);
	printf("# of materials = %d\n", (int)m_materials.size());
	printf("# of shapes    = %d\n", (int)shapes.size());

	// Append `default` material
	m_materials.push_back(tinyobj::material_t());

#if TEX_VCOLOR
	// Load diffuse m_textures
	{
			for (size_t m = 0; m < m_materials.size(); m++) {
					tinyobj::material_t* mp = &m_materials[m];

					if (mp->diffuse_texname.length() > 0) {
							// Only load the texture if it is not already loaded
							if (m_textures.find(mp->diffuse_texname) == m_textures.end()) {
									GLuint texture_id;
									int w, h;
									int comp;

									std::string texture_filename = mp->diffuse_texname;
									if (!FileExists(texture_filename)) {
										// Append base dir.
										texture_filename = base_dir + mp->diffuse_texname;
										if (!FileExists(texture_filename)) {
											std::cerr << "Unable to find file: " << mp->diffuse_texname << std::endl;
											exit(1);
										}
									}

									unsigned char* image = nullptr; //stbi_load(texture_filename.c_str(), &w, &h, &comp, STBI_default);
									if (!image) {
											std::cerr << "Unable to load texture: " << texture_filename << std::endl;
											exit(1);
									}
									glGenTextures(1, &texture_id);
									glBindTexture(GL_TEXTURE_2D, texture_id);
									glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
									glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
									if (comp == 3) {
											glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
									}
									else if (comp == 4) {
											glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
									}
									glBindTexture(GL_TEXTURE_2D, 0);
									//stbi_image_free(image);
									m_textures.insert(std::make_pair(mp->diffuse_texname, texture_id));
							}
					}
			}
	}
#endif

	m_extMin[0] = m_extMin[1] = m_extMin[2] = std::numeric_limits<float>::max();
	m_extMax[0] = m_extMax[1] = m_extMax[2] = -std::numeric_limits<float>::max();

	{
		for (size_t s = 0; s < shapes.size(); s++) {
			DrawObject o;
			std::vector<float> vb;  // pos(3float), normal(3float), color(3float)
			for (size_t f = 0; f < shapes[s].mesh.indices.size() / 3; f++) {
				tinyobj::index_t idx0 = shapes[s].mesh.indices[3 * f + 0];
				tinyobj::index_t idx1 = shapes[s].mesh.indices[3 * f + 1];
				tinyobj::index_t idx2 = shapes[s].mesh.indices[3 * f + 2];

				int current_material_id = shapes[s].mesh.material_ids[f];

				if ((current_material_id < 0) || (current_material_id >= static_cast<int>(m_materials.size()))) {
					// Invaid material ID. Use default material.
					current_material_id = m_materials.size() - 1; // Default material is added to the last item in `m_materials`.
				}
				//if (current_material_id >= m_materials.size()) {
				//    std::cerr << "Invalid material index: " << current_material_id << std::endl;
				//}
				//
#if TEX_VCOLOR
				float diffuse[3];
				for (size_t i = 0; i < 3; i++) {
						diffuse[i] = m_materials[current_material_id].diffuse[i];
				}

				float tc[3][2];

				if (attrib.texcoords.size() > 0) {
						assert(attrib.texcoords.size() > 2 * idx0.texcoord_index + 1);
						assert(attrib.texcoords.size() > 2 * idx1.texcoord_index + 1);
						assert(attrib.texcoords.size() > 2 * idx2.texcoord_index + 1);
						tc[0][0] = attrib.texcoords[2 * idx0.texcoord_index];
						tc[0][1] = 1.0f - attrib.texcoords[2 * idx0.texcoord_index + 1];
						tc[1][0] = attrib.texcoords[2 * idx1.texcoord_index];
						tc[1][1] = 1.0f - attrib.texcoords[2 * idx1.texcoord_index + 1];
						tc[2][0] = attrib.texcoords[2 * idx2.texcoord_index];
						tc[2][1] = 1.0f - attrib.texcoords[2 * idx2.texcoord_index + 1];
				} else {
						tc[0][0] = 0.0f;
						tc[0][1] = 0.0f;
						tc[1][0] = 0.0f;
						tc[1][1] = 0.0f;
						tc[2][0] = 0.0f;
						tc[2][1] = 0.0f;
				}
#endif

				float v[3][3];
				for (int k = 0; k < 3; k++) {
					int f0 = idx0.vertex_index;
					int f1 = idx1.vertex_index;
					int f2 = idx2.vertex_index;
					assert(f0 >= 0);
					assert(f1 >= 0);
					assert(f2 >= 0);

					v[0][k] = attrib.vertices[3 * f0 + k];
					v[1][k] = attrib.vertices[3 * f1 + k];
					v[2][k] = attrib.vertices[3 * f2 + k];
					m_extMin[k] = std::min(v[0][k], m_extMin[k]);
					m_extMin[k] = std::min(v[1][k], m_extMin[k]);
					m_extMin[k] = std::min(v[2][k], m_extMin[k]);
					m_extMax[k] = std::max(v[0][k], m_extMax[k]);
					m_extMax[k] = std::max(v[1][k], m_extMax[k]);
					m_extMax[k] = std::max(v[2][k], m_extMax[k]);
				}

				float n[3][3];
				if (m_bNoNewNormals || (attrib.normals.size() > 0 && m_fScale ==1.0f)) {
					int f0 = idx0.normal_index;
					int f1 = idx1.normal_index;
					int f2 = idx2.normal_index;
					assert(f0 >= 0);
					assert(f1 >= 0);
					assert(f2 >= 0);
					for (int k = 0; k < 3; k++) {
						n[0][k] = attrib.normals[3 * f0 + k];
						n[1][k] = attrib.normals[3 * f1 + k];
						n[2][k] = attrib.normals[3 * f2 + k];
					}
				} else {
					// compute geometric normal
					CalcNormal(n[0], v[0], v[1], v[2]);
					n[1][0] = n[0][0];
					n[1][1] = n[0][1];
					n[1][2] = n[0][2];
					n[2][0] = n[0][0];
					n[2][1] = n[0][1];
					n[2][2] = n[0][2];
				}

				for (int k = 0; k < 3; k++) {
					vb.push_back(v[k][0]);
					vb.push_back(v[k][1]);
					vb.push_back(v[k][2]);
					vb.push_back(n[k][0]);
					vb.push_back(n[k][1]);
					vb.push_back(n[k][2]);
#if TEX_VCOLOR
					// Combine normal and diffuse to get color.
					float normal_factor = 0.2;
					float diffuse_factor = 1 - normal_factor;
					float c[3] = {
							n[k][0] * normal_factor + diffuse[0] * diffuse_factor,
							n[k][1] * normal_factor + diffuse[1] * diffuse_factor,
							n[k][2] * normal_factor + diffuse[2] * diffuse_factor
					};
					float len2 = c[0] * c[0] + c[1] * c[1] + c[2] * c[2];
					if (len2 > 0.0f) {
						float len = sqrtf(len2);

						c[0] /= len;
						c[1] /= len;
						c[2] /= len;
					}
					vb.push_back(c[0] * 0.5 + 0.5);
					vb.push_back(c[1] * 0.5 + 0.5);
					vb.push_back(c[2] * 0.5 + 0.5);

					vb.push_back(tc[k][0]);
					vb.push_back(tc[k][1]);
#endif
				}
			}

			o.vb = 0;
			o.numTriangles = 0;

			// OpenGL viewer does not support texturing with per-face material.
			if (shapes[s].mesh.material_ids.size() > 0 && shapes[s].mesh.material_ids.size() > s) {
					// Base case
					o.material_id = shapes[s].mesh.material_ids[s];
			} else {
					o.material_id = m_materials.size() - 1; // = ID for default material.
			}

			if (vb.size() > 0) {
				glGenBuffers(1, &o.vb);
				glBindBuffer(GL_ARRAY_BUFFER, o.vb);
				glBufferData(GL_ARRAY_BUFFER, vb.size() * sizeof(float), &vb.at(0),
										 GL_STATIC_DRAW);
#if TEX_VCOLOR
				o.numTriangles = vb.size() / ((3 + 3 + 3 + 2) * 3);
#else
				o.numTriangles = vb.size() / ((3 + 3) * 3);
#endif
				printf("shape[%d] # of triangles = %d\n", static_cast<int>(s),
							 o.numTriangles);
			}
			o.bDraw = true;
			m_DrawObjects.push_back(o);
		}
	}

	printf("m_extMin = %f, %f, %f\n", m_extMin[0], m_extMin[1], m_extMin[2]);
	printf("m_extMax = %f, %f, %f\n", m_extMax[0], m_extMax[1], m_extMax[2]);

	return true;
}
