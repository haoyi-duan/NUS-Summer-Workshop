// VERTEX SHADER

#version 330 core

//============================================================================
// Indicates whether
//   true  -- rendering the shadow map, or
//   false -- using the shadow map to render shadows.
//============================================================================
uniform bool RenderShadowMapMode;


//============================================================================
// Vertex attributes.
//============================================================================
layout (location = 0) in vec4 vPosition;  // Vertex position in object space.
layout (location = 1) in vec3 vNormal;    // Vertex normal in object space.
layout (location = 2) in vec2 vTexCoord;  // Vertex's 2D texture coordinates.


//============================================================================
// View and projection matrices, etc.
//============================================================================
uniform mat4 ModelMatrix;         // Modelling transformation matrix.
uniform mat4 ModelViewMatrix;     // ModelView matrix.
uniform mat4 ModelViewProjMatrix; // ModelView matrix * Projection matrix.
uniform mat3 NormalMatrix;        // For transforming object-space direction
                                  //   vector to eye space.


//============================================================================
// To be output to rasterizer and fragment shader.
//============================================================================
out vec3 wcPosition;   // Vertex position in world space.
out vec3 ecPosition;   // Vertex position in eye space.
out vec3 ecNormal;     // Vertex normal in eye space.
out vec2 texCoord;     // Vertex's texture coordinates.


void main()
{
    if ( !RenderShadowMapMode ) {
        wcPosition = vec3( ModelMatrix * vPosition );
        ecPosition = vec3( ModelViewMatrix * vPosition );
        ecNormal = normalize( NormalMatrix * vNormal );
        texCoord = vTexCoord;
    }
    gl_Position = ModelViewProjMatrix * vPosition;
}
