#pragma once

#include <vector>

class PLYExporter {

public:
    using VF = std::vector<float>; // this is what the HR renderer uses for
    using VI = std::vector<int>; // these are the indices for GL_TRIANGLE_STRIP and triangle counts
    
    PLYExporter() = default;
    ~PLYExporter() = default;
    
    bool Export(const char *fname, const VF &tri, const VF &triNorm, const VF& triColor, const VI &tStart, const VI &tCount);
};
