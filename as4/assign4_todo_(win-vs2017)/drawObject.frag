// FRAGMENT SHADER

#version 330 core

//============================================================================
// Received from vertex shader and rasterizer.
//============================================================================
in vec3 ecPosition;  // Fragment's 3D position in eye space.
in vec3 ecNormal;    // Fragment's normal vector in eye space.
in vec2 texCoord;    // Fragment's texture coordinates.

//============================================================================
// Light info.
//============================================================================
uniform vec3 LightPosition; // Given in eye space. Must be point light.
uniform vec3 LightAmbient;
uniform vec3 LightDiffuse;
uniform vec3 LightSpecular;

//============================================================================
// Material info.
//============================================================================
uniform vec3 MatlAmbient;
uniform vec3 MatlDiffuse;
uniform vec3 MatlSpecular;
uniform float MatlShininess;

//============================================================================
// Texture maps.
//============================================================================
uniform sampler2D AmbientDiffuseTex;  // Always at Texture Unit 0.

uniform bool UseAmbientDiffuseTex;  // Is the object using texture map?

//============================================================================
// Outputs.
//============================================================================
layout (location = 0) out vec4 FragColor;


/////////////////////////////////////////////////////////////////////////////
// Computes and returns the values of N_dot_L and R_dot_V_pow_n
// in the Phong Illumination Equation.
/////////////////////////////////////////////////////////////////////////////
void Phong(out float N_dot_L, out float R_dot_V_pow_n)
{
    vec3 V = normalize( -ecPosition );
    vec3 N = normalize( ecNormal );
    vec3 L = normalize( LightPosition - ecPosition );
    vec3 R = reflect( -L, N );
    N_dot_L = max( 0.0, dot( N, L ) );
    float R_dot_V = max( 0.0, dot( R, V ) );
    R_dot_V_pow_n = ( R_dot_V == 0.0 )? 0.0 : pow( R_dot_V, MatlShininess );
}


void main()
{
    float N_dot_L, R_dot_V_pow_n;
    Phong( N_dot_L, R_dot_V_pow_n );

    if ( UseAmbientDiffuseTex )
    {
        vec3 MatlAmbientDiffuse = texture( AmbientDiffuseTex, texCoord.st ).rgb;
        FragColor.rgb = LightAmbient * MatlAmbientDiffuse +
                        LightDiffuse * MatlAmbientDiffuse * N_dot_L +
                        LightSpecular * MatlSpecular * R_dot_V_pow_n;
        FragColor.a = 1.0;
    }
    else
    {
        FragColor.rgb = LightAmbient * MatlAmbient +
                        LightDiffuse * MatlDiffuse * N_dot_L +
                        LightSpecular * MatlSpecular * R_dot_V_pow_n;
        FragColor.a = 1.0;
    }
}
