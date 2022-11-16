#ifndef VBOSPHERE2_H
#define VBOSPHERE2_H

#include "drawable.h"
#include "gldecl.h"


class VBOSphere2 : public Drawable
{
private:
    unsigned int vaoHandle;
    GLuint nVerts, elements;
	float radius;
	GLuint slices, stacks;

    void generateVerts(float * , float * ,float *, GLuint *);

public:
	VBOSphere2(float rad, GLuint sl, GLuint st);

    void render() const;

    int getVertexArrayHandle();
};

#endif // VBOSPHERE2_H
