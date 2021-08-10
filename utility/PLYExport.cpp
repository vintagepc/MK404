/*
	PLYExporter.cpp - PLY export helper for GLPrint

	Copyright 2020 DRracer <https://github.com/DRracer/>

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


#include "PLYExport.h"
#include "Config.h"
#include "gsl-lite.hpp"
#include <cstddef>
#include <cstdint>
#include <fstream>

bool PLYExporter::Export(const std::string& strFN, const VF &tri, const VF &triNorm, const VF& triColor, const VI &tStart, const VI &tCount){

    std::ofstream f(strFN);

	const bool bColourEnabled = Config::Get().GetColourE();

    if(!f.is_open()) return false;

    // verifying equal sizes of individual containers makes the iteration simpler then
    if( tri.size() != triNorm.size() ) return false;

    if(bColourEnabled && tri.size() != triColor.size() ) return false;

    // this part can be probably optimized...
    size_t numOfTri = 0;
	for (auto &cnt: tCount)
	{
		if (cnt>=3)
		{
			numOfTri+=cnt-2;
		}
	}

    f <<
        "ply\n"
        "format ascii 1.0\n"
        "element vertex " << tri.size() / 3 <<
        "\nproperty float x\n"
        "property float y\n"
        "property float z\n"
        "property float nx\n"
        "property float ny\n"
        "property float nz\n";
	if (bColourEnabled)
	{
		f <<
			"property uchar red\n"
			"property uchar green\n"
			"property uchar blue\n";
	}
	f <<
        "element face " << numOfTri <<
        "\nproperty list uchar int vertex_index\n"
        "end_header\n";

    { // dump vertices (which is basically the raw data we have in tri along with normal and color components
        auto ti = tri.cbegin(), tni = triNorm.cbegin(), tci = triColor.cbegin();
        for( ; ti != tri.cend(); ){
            f << *ti << ' '; ++ti; // x
            f << *ti << ' '; ++ti; // y
            f << *ti << ' '; ++ti; // z
            f << *tni << ' '; ++tni; // normal x
            f << *tni << ' '; ++tni; // normal y
            f << *tni << ' '; ++tni; // normal z
		if (bColourEnabled)
		{
            f << uint16_t( *tci * 255) << ' '; ++tci; // color R
            f << uint16_t( *tci * 255) << ' '; ++tci; // color G
            f << uint16_t( *tci * 255) << ' '; ++tci; // color B
		}
            f << '\n';
        }
    }
    // now we have to mimic the behavior of this GL procedure
    //glVertexPointer(3, GL_FLOAT, 3*sizeof(float), m_fvTri.data());
    //glNormalPointer(GL_FLOAT, 3*sizeof(float), m_fvTriNorm.data());
    //if (m_bColExt)
    //{
    //    glEnable(GL_COLOR_MATERIAL);
    //    glEnableClientState(GL_COLOR_ARRAY);
    //    glColorPointer(3, GL_FLOAT, 3*sizeof(float), m_vfTriColor.data());
    //}
    //glMultiDrawArrays(GL_TRIANGLE_STRIP,m_ivTStart.data(),m_ivTCount.data(), m_ivTCount.size());

    // start indices are in tStart
    // their count is in tCount at the same index
    {
        auto tsi = tStart.cbegin();
        auto tci = tCount.cbegin();
        for( ; tsi != tStart.end(); ++tsi, ++tci ){
            int stripStartVertexIndex = *tsi;
            auto stripVertexCount = gsl::narrow<uint32_t>(*tci); // really should be >=3
            if( stripVertexCount < 3 )
			{
                continue;
			}

            // from the man pages of GL_TRIANGLE_STRIP:
            // Draws a connected group of triangles.
            // One triangle is defined for each vertex presented after the first two vertices.
            // For odd n, vertices n, n+1, and n+2 define triangle n.
            // For even n, vertices n+1, n, and n+2 define triangle n. N-2 triangles are drawn.
            for(size_t n = 0; n < stripVertexCount - 2; ++n){
                if( ( n & 1u) == 0 ){
                    // even
                    f << "3 " << stripStartVertexIndex + n
                      << ' ' << stripStartVertexIndex + n + 1
                      << ' ' << stripStartVertexIndex + n + 2 << '\n';
                } else {
                    // odd
                    f << "3 " << stripStartVertexIndex + n + 1
                      << ' ' << stripStartVertexIndex + n
                      << ' ' << stripStartVertexIndex + n + 2 << '\n';
                }
            }
        }
    }
	f.flush();
	f.close();
    return true;
}
