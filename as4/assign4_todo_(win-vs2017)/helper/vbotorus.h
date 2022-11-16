#ifndef VBOTORUS_H
#define VBOTORUS_H

#include "drawable.h"

class VBOTorus : public Drawable
{
private:
    unsigned int vaoHandle;
    int faces, rings, sides;

    void generateVerts(float * , float * ,float *, unsigned int *,
                       float , float);

public:
    VBOTorus(float outerRadius, float innerRadius, int nsides, int nrings);

    void render() const;

	int getVertexArrayHandle();
};

#endif // VBOTORUS_H
