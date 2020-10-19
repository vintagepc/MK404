#include "PLYExport.h"
#include <fstream>

bool PLYExporter::Export(const char *fname, const VF &tri, const VF &triNorm, const VF& triColor, const VI &tStart, const VI &tCount){
    
    std::ofstream f(fname);
    if(!f.is_open())return false;
    
    // verifying equal sizes of individual containers makes the iteration simpler then
    if( tri.size() != triNorm.size() ) return false;
    if( tri.size() != triColor.size() ) return false;

    // this part can be probably optimized...
    size_t numOfTri = 0;
    {
        auto tci = tCount.cbegin();
        for( ; tci != tCount.end(); ++tci ){
            int stripVertexCount = *tci; // really should be >=3
            if( stripVertexCount < 3 )
                continue;
            // starting triangle
            ++numOfTri;
            // consecutive triangles
            for(int i = 3; i < stripVertexCount; ++i){
                ++numOfTri;
            }
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
        "property float nz\n"
        "property uchar red\n"
        "property uchar green\n"
        "property uchar blue\n"
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
            f << uint16_t( *tci * 255) << ' '; ++tci; // color R
            f << uint16_t( *tci * 255) << ' '; ++tci; // color G
            f << uint16_t( *tci * 255) << ' '; ++tci; // color B
            f << std::endl;
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
            int stripVertexCount = *tci; // really should be >=3
            if( stripVertexCount < 3 )
                continue;
            
            // from the man pages of GL_TRIANGLE_STRIP:
            // Draws a connected group of triangles. 
            // One triangle is defined for each vertex presented after the first two vertices. 
            // For odd n, vertices n, n+1, and n+2 define triangle n. 
            // For even n, vertices n+1, n, and n+2 define triangle n. N-2 triangles are drawn. 
            for(int n = 0; n < stripVertexCount - 2; ++n){
                if( ( n & 1) == 0 ){
                    // even
                    f << "3 " << stripStartVertexIndex + n
                      << ' ' << stripStartVertexIndex + n + 1
                      << ' ' << stripStartVertexIndex + n + 2 << std::endl;  
                } else {
                    // odd
                    f << "3 " << stripStartVertexIndex + n + 1
                      << ' ' << stripStartVertexIndex + n
                      << ' ' << stripStartVertexIndex + n + 2 << std::endl;  
                }
            }
        }
    }
    return true;
}
