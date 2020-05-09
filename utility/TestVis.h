
#include <vector>
#include <tiny_obj_loader.h>
#include <GL/glew.h>



class TestVis 
{
    public:
        void Draw();
        void Load();
    private:
        typedef struct {
            GLuint vb;  // vertex buffer
            int numTriangles;
            size_t material_id;
        } DrawObject;
        float bmin[3], bmax[3];
        std::vector<DrawObject> gDrawObjects;
        std::vector<tinyobj::material_t> materials;
        std::map<std::string, GLuint> textures;

        float eye[3], lookat[3], up[3], maxExtent;
        void _Draw(const std::vector<TestVis::DrawObject>& drawObjects, std::vector<tinyobj::material_t>& materials, std::map<std::string, GLuint>& textures); 
        bool LoadObjAndConvert(float bmin[3], float bmax[3],
                       std::vector<DrawObject>* drawObjects,
                       std::vector<tinyobj::material_t>& materials,
                       std::map<std::string, GLuint>& textures,
                       const char* filename);
};