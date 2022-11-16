// VERTEX SHADER

#version 330 core

//============================================================================
// Vertex attributes.
//============================================================================
layout (location = 0) in vec4 vPosition;  // Vertex position in object space.
layout (location = 1) in vec3 vNormal;    // Vertex normal in object space.
layout (location = 2) in vec2 vTexCoord;  // Vertex's 2D texture coordinates.

//============================================================================
// Indicates which object is being rendered.
// 0 -- draw skybox, 1 -- draw brick cube, 2 -- draw wooden cube.
//============================================================================
uniform int WhichObj;

//============================================================================
// View and projection matrices, etc.
//============================================================================
uniform mat4 ViewMatrix;          // View transformation matrix.
uniform mat4 ModelViewMatrix;     // ModelView matrix.
uniform mat4 ModelViewProjMatrix; // ModelView matrix * Projection matrix.
uniform mat3 NormalMatrix;        // For transforming object-space direction
                                  //   vector to eye space.

//============================================================================
// To be output to rasterizer and fragment shader.
//============================================================================
out vec3 ecPosition;   // Vertex position in eye space.
out vec3 ecNormal;     // Vertex normal in eye space.
out vec3 v2fTexCoord;  // Vertex's texture coordinates. It is 3D when it is
                       //   used as texture coordinates to a cubemap.


void main()
{
    switch (WhichObj) {
        case 0: { // skybox
            v2fTexCoord = normalize(vPosition.xyz);
            break;
        }
        case 1: { // brick cube
            v2fTexCoord = vec3(vTexCoord, 0.0);
            ecNormal = normalize( NormalMatrix * vNormal );
            vec4 ecPosition4 = ModelViewMatrix * vPosition;
            ecPosition = vec3( ecPosition4 ) / ecPosition4.w;
            break;
        }
        case 2: { // wooden cube
            v2fTexCoord = vec3(vTexCoord, 0.0);
            ecNormal = normalize( NormalMatrix * vNormal );
            vec4 ecPosition4 = ModelViewMatrix * vPosition;
            ecPosition = vec3( ecPosition4 ) / ecPosition4.w;
            break;
        }
    }

    gl_Position = ModelViewProjMatrix * vPosition;
}
