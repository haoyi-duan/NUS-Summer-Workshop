#ifndef VBOSPHERE_H
#define VBOSPHERE_H

#include "drawable.h"
#include "gldecl.h"

class VBOSphere : public Drawable
{
private:
    unsigned int vaoHandle;
    GLuint nVerts, elements;
	float radius;
	GLuint slices, stacks;

    void generateVerts(float * , float * ,float *, GLuint *);

public:
    VBOSphere(float rad, GLuint sl, GLuint st);

    void render() const;

    int getVertexArrayHandle();
};

#endif // VBOSPHERE_H
