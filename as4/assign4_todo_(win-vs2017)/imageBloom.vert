// VERTEX SHADER

#version 330 core

//============================================================================
// Vertex attributes.
//============================================================================
layout (location = 0) in vec3 vPosition;  // Vertex position in clip space or NDC.
layout (location = 2) in vec2 vTexCoord;  // Vertex's 2D texture coordinates.

//============================================================================
// To be output to rasterizer and fragment shader.
//============================================================================
out vec2 texCoord;  // Texture coordinates.


void main()
{
    texCoord = vTexCoord;
    gl_Position =  vec4( vPosition, 1.0 );
    // No need to transform because vertices of the quad are given in NDC.
}
