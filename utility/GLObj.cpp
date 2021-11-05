/*
	GLObj.cpp - a GL wrangler class for .obj files/visuals.

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


#include "GLObj.h"
#include "gsl-lite.hpp"
#include "tiny_obj_loader.h"  // for attrib_t, index_t, mesh_t, shape_t, Loa...
#include <GL/glew.h>          // for glMaterialfv, GL_FRONT, glBindTexture
#include <algorithm>          // for max, min
#include <cmath>              // for sqrtf
#include <cstring>            // for memcpy
#include <functional>
#include <iostream>           // for operator<<, endl, basic_ostream, std::cerr
#include <limits>             // for numeric_limits
#include <map>                // for map, _Rb_tree_iterator
#include <memory>             // for allocator_traits<>::value_type
#include <string>             // for string, operator<<, char_traits
#include <utility>
#include <vector>             // for vector


// This disables textures and vertex colors, not used in favor of materials anyway.
// Also cuts GPU RAM usage in half, and probably has performance gains for not needing to set the vertex properties.
#define TEX_VCOLOR 0

GLObj::GLObj(std::string strFile,  float fTX, float fTY, float fTZ, float fScale):m_strFile(std::move(strFile)),m_fScale(fScale),m_fCorr{fTX,fTY,fTZ}
{
}

GLObj::GLObj(std::string strFile, float fScale):m_strFile(std::move(strFile)),m_fScale(fScale)
{
}
GLObj::GLObj(std::string strFile):m_strFile(std::move(strFile))
{
}


void GLObj::Load()
{
	m_bLoaded = LoadObjAndConvert(m_strFile.c_str());
	if (!m_bLoaded)
	{
		std::cout << "Failed to load obj\n";
	}

	m_fMaxExtent = 0.5f * (m_extMax[0] - m_extMin[0]);
	if (m_fMaxExtent < 0.5f * (m_extMax[1] - m_extMin[1]))
	{
			m_fMaxExtent = 0.5f * (m_extMax[1] - m_extMin[1]);
	}
	if (m_fMaxExtent < 0.5f * (m_extMax[2] - m_extMin[2]))
	{
			m_fMaxExtent = 0.5f * (m_extMax[2] - m_extMin[2]);
	}
}

void GLObj::SetAllVisible(bool bVisible)
{
	std::lock_guard<std::mutex> lock(m_lock);
	for (auto &obj : m_DrawObjects)
	{
		obj.bDraw = bVisible;
	}
}

void GLObj::SetSubobjectVisible(unsigned iObj, bool bVisible)
{
	std::lock_guard<std::mutex> lock(m_lock);
	if (iObj<m_DrawObjects.size())
	{
		m_DrawObjects[iObj].bDraw = bVisible;
	}
}

void GLObj::SetSubobjectMaterial(unsigned iObj, unsigned iMat)
{
	if (iObj<m_DrawObjects.size() && iMat < m_materials.size())
	{
		std::lock_guard<std::mutex> lock(m_lock);
		//printf("Changed obj %d to %d\n",iObj,iMat);
		m_DrawObjects[iObj].material_id = iMat;
	}
	else
	{
		std::cout << "GLObj: Tried to set invalid material or object.\n"; // pragma: LCOV_EXCL_LINE
	}
}

void GLObj::SwapMaterial(unsigned iOld, unsigned iNew)
{
	if (iNew < m_materials.size())
	{
		std::lock_guard<std::mutex> lock(m_lock);
		for (auto &m : m_DrawObjects)
		{
			if (m.material_id == iOld)
			{
				m.material_id = iNew;
			}
		}
	}
	else
	{
		std::cout << "GLObj: Invalid new material ID\n"; // pragma: LCOV_EXCL_LINE
	}

}

void GLObj::Draw() {
	if (!m_bLoaded)	return;

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
	{
		glRotatef(-90,1,0,0);
	}
	std::lock_guard<std::mutex> lock(m_lock);
	for (auto o : m_DrawObjects)
	{
		if (o.vb < 1 || !o.bDraw)
		{
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
			if (m_textures.find(diffuse_texname) != m_textures.end())
			{
				glBindTexture(GL_TEXTURE_2D, m_textures[diffuse_texname]);
			} else {
				const tinyobj::material_t pMat = gsl::at(m_materials,o.material_id);
				float _fCopy[4] = {0,0,0,pMat.dissolve};
				gsl::span<float> fCopy {_fCopy};
				memcpy(fCopy.data(),static_cast<const float*>(pMat.ambient),3*(sizeof(float)));
				glMaterialfv(GL_FRONT, GL_AMBIENT,  fCopy.begin());
				memcpy(fCopy.data(),static_cast<const float*>(pMat.diffuse),3*(sizeof(float)));
				glMaterialfv(GL_FRONT, m_matMode, fCopy.begin());
				memcpy(fCopy.data(),static_cast<const float*>(pMat.specular),3*(sizeof(float)));
				glMaterialfv(GL_FRONT, GL_SPECULAR, fCopy.begin());
				glMaterialf(GL_FRONT, GL_SHININESS, (pMat.shininess/1000.f)*128.f);
				glMaterialfv(GL_FRONT, GL_EMISSION, static_cast<const float*>(pMat.emission));
			}

		}
		glVertexPointer(3, GL_FLOAT, stride, nullptr);
		glNormalPointer(GL_FLOAT, stride, (const void*)((sizeof(float) * 3))); //NOLINT it is what it is.
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
	{
		return filepath.substr(0, filepath.find_last_of("/\\"));
	}
	return "";
}

// Calculates a face normal for the given triangle.
static void CalcNormal(gsl::span<float> N, gsl::span<float> v0, gsl::span<float> v1, gsl::span<float>v2)
{
	float _v10[3];
	float _v20[3];
	gsl::span<float> v10(_v10), v20(_v20);

	std::transform(v1.begin(), v1.end(), v0.begin(), v10.data(),std::minus<float>());
	std::transform(v2.begin(), v2.end(), v0.begin(), v20.data(),std::minus<float>());

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
		std::cerr << err << '\n';
	}

	if (!ret) {
		std::cerr << "Failed to load " << filename << '\n';
		return false;
	}
	for (auto &vert : attrib.vertices)
	{
		vert *= m_fScale;
	}

	std::cout << "##### " << filename <<" #####\n";
	std::cout << "# of vertices  = " << (attrib.vertices.size()) / 3 << '\n';
	std::cout << "# of normals   = " << (attrib.normals.size()) / 3 << '\n';
	std::cout << "# of texcoords = " << (attrib.texcoords.size()) / 2 << '\n';
	std::cout << "# of materials = " << m_materials.size() << '\n';
	std::cout << "# of shapes    = " << shapes.size() << '\n';

	// Append `default` material
	m_materials.push_back(tinyobj::material_t());

	if (m_bSetDissolve)
	{
		for (auto &m : m_materials)
		{
			if (m.dissolve == 0.f) m.dissolve = 1.0f;
		}
	}

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
											std::cerr << "Unable to find file: " << mp->diffuse_texname << '\n';
											exit(1);
										}
									}

									unsigned char* image = nullptr; //stbi_load(texture_filename.c_str(), &w, &h, &comp, STBI_default);
									if (!image) {
											std::cerr << "Unable to load texture: " << texture_filename << '\n';
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
		for (auto &s : shapes) {
			std::vector<float> vb;  // pos(3float), normal(3float), color(3float)
			int iMatlId = s.mesh.material_ids[0];
			for (size_t f = 0; f < s.mesh.indices.size() / 3; f++) {
				tinyobj::index_t idx0 = s.mesh.indices[3 * f + 0];
				tinyobj::index_t idx1 = s.mesh.indices[3 * f + 1];
				tinyobj::index_t idx2 = s.mesh.indices[3 * f + 2];

				int current_material_id = s.mesh.material_ids[f];

				if (current_material_id != iMatlId)
				{
					//printf("Submaterial in shape %s: %u\n",s.name.c_str(),current_material_id);
					AddObject(vb,iMatlId);
					vb.clear();
				}

				if ((current_material_id < 0) || (current_material_id >= static_cast<int>(m_materials.size()))) {
					// Invaid material ID. Use default material.
					iMatlId = m_materials.size() - 1; // Default material is added to the last item in `m_materials`.
				}
				else
				{
					iMatlId = current_material_id;
				}

				//if (current_material_id >= m_materials.size()) {
				//    std::cerr << "Invalid material index: " << current_material_id << '\n';
				//}
				//
#if TEX_VCOLOR
				float diffuse[3];
				for (size_t i = 0; i < 3; i++) {
						diffuse[i] = m_materials[current_material_id].diffuse[i];
				}

				float tc[3][2];

				if (attrib.texcoords.size() > 0) {
						Expects(attrib.texcoords.size() > 2 * idx0.texcoord_index + 1);
						Expects(attrib.texcoords.size() > 2 * idx1.texcoord_index + 1);
						Expects(attrib.texcoords.size() > 2 * idx2.texcoord_index + 1);
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
					Expects(f0 >= 0);
					Expects(f1 >= 0);
					Expects(f2 >= 0);

					gsl::at(v[0],k) = attrib.vertices[3 * f0 + k];
					gsl::at(v[1],k) = attrib.vertices[3 * f1 + k];
					gsl::at(v[2],k) = attrib.vertices[3 * f2 + k];
					m_extMin[k] = std::min(gsl::at(v[0],k), m_extMin[k]);
					m_extMin[k] = std::min(gsl::at(v[1],k), m_extMin[k]);
					m_extMin[k] = std::min(gsl::at(v[2],k), m_extMin[k]);
					m_extMax[k] = std::max(gsl::at(v[0],k), m_extMax[k]);
					m_extMax[k] = std::max(gsl::at(v[1],k), m_extMax[k]);
					m_extMax[k] = std::max(gsl::at(v[2],k), m_extMax[k]);
				}

				float n[3][3];
				if (m_bNoNewNormals || (attrib.normals.size() > 0 && m_fScale ==1.0f)) {
					int f0 = idx0.normal_index;
					int f1 = idx1.normal_index;
					int f2 = idx2.normal_index;
					Expects(f0 >= 0);
					Expects(f1 >= 0);
					Expects(f2 >= 0);
					for (int k = 0; k < 3; k++) {
						gsl::at(n[0],k) = attrib.normals[3 * f0 + k];
						gsl::at(n[1],k) = attrib.normals[3 * f1 + k];
						gsl::at(n[2],k) = attrib.normals[3 * f2 + k];
					}
				} else {
					// compute geometric normal
					if (m_bReverseNormals)
					{
						CalcNormal(n[0], v[0], v[2], v[1]);
					}
					else
					{
						CalcNormal(n[0], v[0], v[1], v[2]);
					}
					n[1][0] = n[0][0];
					n[1][1] = n[0][1];
					n[1][2] = n[0][2];
					n[2][0] = n[0][0];
					n[2][1] = n[0][1];
					n[2][2] = n[0][2];
				}

				for (int k = 0; k < 3; k++) {
					vb.push_back(gsl::at(v,k)[0]);
					vb.push_back(gsl::at(v,k)[1]);
					vb.push_back(gsl::at(v,k)[2]);
					vb.push_back(gsl::at(n,k)[0]);
					vb.push_back(gsl::at(n,k)[1]);
					vb.push_back(gsl::at(n,k)[2]);
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
			//printf("Object %s: is # %u\n",s.name.c_str(),(int)m_DrawObjects.size());
			AddObject(vb, iMatlId);
			// // OpenGL viewer does not support texturing with per-face material.
			// if (s.mesh.material_ids.size() > 0 && s.mesh.material_ids.size() > s) {
			// 		// Base case
			// 		o.material_id = s.mesh.material_ids[s];
			// } else {
			// 		o.material_id = m_materials.size() - 1; // = ID for default material.
			// }


		}
	}

	// printf("m_extMin = %f, %f, %f\n", m_extMin[0], m_extMin[1], m_extMin[2]);
	// printf("m_extMax = %f, %f, %f\n", m_extMax[0], m_extMax[1], m_extMax[2]);

	return true;
}

void GLObj::AddObject(const std::vector<float> &vb, int iMatlId)
{
	DrawObject obj {};
	obj.vb = 0;
	obj.numTriangles = 0;
	if (vb.size() > 0) {
		glGenBuffers(1, &obj.vb);
		glBindBuffer(GL_ARRAY_BUFFER, obj.vb);
		glBufferData(GL_ARRAY_BUFFER, vb.size() * sizeof(float), &vb.at(0),
									GL_STATIC_DRAW);
#if TEX_VCOLOR
		obj.numTriangles = vb.size() / ((3 + 3 + 3 + 2) * 3);
#else
		obj.numTriangles = vb.size() / ((3 + 3) * 3);
#endif
		// printf("shape[%d] # of triangles = %d\n", static_cast<int>(s), obj.numTriangles);
	}
	obj.bDraw = true;
	obj.material_id = iMatlId;
	m_DrawObjects.push_back(obj);
}
