
#include <vector>
#include <type_traits>
#include <tiny_obj_loader.h>
#include <GL/glew.h>

class GLObj 
{
    public:
        GLObj(std::string strFile);
        void Draw();
        void SetSubobjectVisible(uint iObj, bool bVisible = true);
        void SetAllVisible(bool bVisible = true);
        float GetScaleFactor() { return m_fMaxExtent; };
        float GetCenteringTransform(float fTrans[3]) { for (int i=0; i<3; i++) fTrans[i] = -0.5f * (m_extMin[i] + m_extMax[i]);}
    private:
        typedef struct {
            GLuint vb;  // vertex buffer
            int numTriangles;
            size_t material_id;
            bool bDraw;
        } DrawObject;
        float m_fMaxExtent;
        float m_extMin[3], m_extMax[3];
        std::vector<tinyobj::material_t> m_materials;
        std::map<std::string, GLuint> m_textures;        
        std::vector<DrawObject> m_DrawObjects;

        bool LoadObjAndConvert(const char* filename);
};