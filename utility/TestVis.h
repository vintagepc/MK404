
#include <vector>
#include <tiny_obj_loader.h>
#include <GL/glew.h>



class TestVis 
{
    public:
        void Draw();
    private:
        typedef struct {
            GLuint vb;  // vertex buffer
            int numTriangles;
            size_t material_id;
        } DrawObject;

        std::vector<DrawObject> gDrawObjects;

        bool LoadObjAndConvert(float bmin[3], float bmax[3],
                       std::vector<DrawObject>* drawObjects,
                       std::vector<tinyobj::material_t>& materials,
                       std::map<std::string, GLuint>& textures,
                       const char* filename);
};