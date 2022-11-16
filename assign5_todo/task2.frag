//============================================================================
// PROJECT ID: 1
//
// GROUP NUMBER:
//
// STUDENT NAME:
// NUS User ID.:
//
// STUDENT NAME:
// NUS User ID.:
//
// STUDENT NAME: Duan Haoyi
// NUS User ID.: t0925750
//
// COMMENTS TO GRADER:
//
//============================================================================


// FRAGMENT SHADER FOR SHADERTOY
// Run this at https://www.shadertoy.com/new
// See documentation at https://www.shadertoy.com/howto

// Your browser must support WebGL 2.0.
// Check your browser at https://webglreport.com/?v=2


//============================================================================
// Constants.
//============================================================================

const float PI = 3.1415926536;

const vec3 BACKGROUND_COLOR = vec3( 0.1, 0.2, 0.6 );

// Vertical field-of-view angle of camera. In radians.
const float FOVY = 50.0 * PI / 180.0;

// Use this for avoiding the "epsilon problem" or the shadow acne problem.
const float DEFAULT_TMIN = 10.0e-4;

// Use this for tmax for non-shadow ray intersection test.
const float DEFAULT_TMAX = 10.0e6;

// Equivalent to number of recursion levels (0 means ray-casting only).
// We are using iterations to replace recursions.
const int NUM_ITERATIONS = 2;

// Constants for the scene objects.
const int NUM_LIGHTS = 2;
const int NUM_MATERIALS = 10;
const int NUM_PLANES = 4;
const int NUM_SPHERES = 9;

//Parameters for block
float blockNum = 8.65;
float blockLength = 0.45;

#define DISCO_COLOR 9
#define BOUNCING_BALL_DIS 3.0
//============================================================================
// Define new struct types.
//============================================================================
struct Ray_t {
    vec3 o;  // Ray Origin.
    vec3 d;  // Ray Direction. A unit vector.
};

struct Plane_t {
    // The plane equation is Ax + By + Cz + D = 0.
    float A, B, C, D;
    int materialID_1;
    int materialID_2;
    int type; // 0 is for normal plane, 1 is for grid.
};

struct Sphere_t {
    vec3 center;
    float radius;
    int materialID;
};

struct Light_t {
    vec3 position;  // Point light 3D position.
    vec3 I_a;       // For Ambient.
    vec3 I_source;  // For Diffuse and Specular.
};

struct Material_t {
    vec3 k_a;   // Ambient coefficient.
    vec3 k_d;   // Diffuse coefficient.
    vec3 k_r;   // Reflected specular coefficient.
    vec3 k_rg;  // Global reflection coefficient.
    float n;    // The specular reflection exponent. Ranges from 0.0 to 128.0.
};

//----------------------------------------------------------------------------
// The lighting model used here is similar to that shown in
// Lecture Topic B08 (Basic Ray Tracing). Here it is computed as
//
//     I_local = SUM_OVER_ALL_LIGHTS {
//                   I_a * k_a +
//                   k_shadow * I_source * [ k_d * (N.L) + k_r * (R.V)^n ]
//               }
// and
//     I = I_local  +  k_rg * I_reflected
//----------------------------------------------------------------------------


//============================================================================
// Global scene data.
//============================================================================
Plane_t Plane[NUM_PLANES];
Sphere_t Sphere[NUM_SPHERES];
Light_t Light[NUM_LIGHTS];
Material_t Material[NUM_MATERIALS];


/////////////////////////////////////////////////////////////////////////////
// Initializes the scene.
/////////////////////////////////////////////////////////////////////////////
void InitScene()
{
    // Horizontal plane.
    Plane[0].A = 0.0;
    Plane[0].B = 1.0;
    Plane[0].C = 0.0;
    Plane[0].D = 0.0;
    Plane[0].materialID_1 = 0;
    Plane[0].materialID_2 = 9;
    Plane[0].type = 1;
    
    // Vertical plane.
    Plane[1].A = 0.0;
    Plane[1].B = 0.0;
    Plane[1].C = 1.0;
    Plane[1].D = 10.0;
    Plane[1].materialID_1 = 9;
    Plane[1].type = 0;

    // left plane
    Plane[2].A = 1.0;
    Plane[2].B = 0.0;
    Plane[2].C = 0.0;
    Plane[2].D = 10.0;
    Plane[2].materialID_1 = 4;
    Plane[2].type = 0;
    
    // right plane
    Plane[3].A = -1.0;
    Plane[3].B = 0.0;
    Plane[3].C = 0.0;
    Plane[3].D = 10.0;
    Plane[3].materialID_1 = 4;
    Plane[3].type = 0;

    // Center bouncing sphere.
    Sphere[0].center = vec3( 0.0, 4. * abs(sin(2.*iTime)) + 0.8, 0.0 );
    Sphere[0].radius = 1.2;
    Sphere[0].materialID = 0;
    
    Sphere[1].center = vec3( BOUNCING_BALL_DIS, 4. *abs(sin(2.*iTime+1.0)) + 0.8, BOUNCING_BALL_DIS );
    Sphere[1].radius = 1.2;
    Sphere[1].materialID = 1;
    
    Sphere[2].center = vec3( -BOUNCING_BALL_DIS, 4. *abs(sin(2.*iTime-1.0)) + 0.8, -BOUNCING_BALL_DIS );
    Sphere[2].radius = 1.2;
    Sphere[2].materialID = 2;
    
    Sphere[3].center = vec3( BOUNCING_BALL_DIS, 4. *abs(sin(2.*iTime+0.5)) + 0.8, 0.0 );
    Sphere[3].radius = 1.2;
    Sphere[3].materialID = 3;
    
    Sphere[4].center = vec3( 0.0, 4. *abs(sin(2.*iTime+0.5)) + 0.8, BOUNCING_BALL_DIS );
    Sphere[4].radius = 1.2;
    Sphere[4].materialID = 4;
    
    Sphere[5].center = vec3( -BOUNCING_BALL_DIS, 4. *abs(sin(2.*iTime-0.5)) + 0.8, 0.0 );
    Sphere[5].radius = 1.2;
    Sphere[5].materialID = 5;
    
    Sphere[6].center = vec3( 0.0, 4. *abs(sin(2.*iTime-0.5)) + 0.8, -BOUNCING_BALL_DIS );
    Sphere[6].radius = 1.2;
    Sphere[6].materialID = 6;
    
    Sphere[7].center = vec3( BOUNCING_BALL_DIS, 4. *abs(sin(2.*iTime)) + 0.8, -BOUNCING_BALL_DIS );
    Sphere[7].radius = 1.2;
    Sphere[7].materialID = 7;
    
    Sphere[8].center = vec3( -BOUNCING_BALL_DIS, 4. *abs(sin(2.*iTime)) + 0.8, BOUNCING_BALL_DIS );
    Sphere[8].radius = 1.2;
    Sphere[8].materialID = 8;
    
    // Silver material.
    Material[0].k_d = vec3( 0.5, 0.5, 0.5 );
    Material[0].k_a = 0.2 * Material[0].k_d;
    Material[0].k_r = 2.0 * Material[0].k_d;
    Material[0].k_rg = 0.5 * Material[0].k_r;
    Material[0].n = 64.0;

    // Gold material.
    Material[1].k_d = vec3( 0.8, 0.7, 0.1 );
    Material[1].k_a = 0.2 * Material[1].k_d;
    Material[1].k_r = 2.0 * Material[1].k_d;
    Material[1].k_rg = 0.5 * Material[1].k_r;
    Material[1].n = 64.0;

    // Green plastic material.
    Material[2].k_d = vec3( 0.4, 0.8, 0.2 );
    Material[2].k_a = 0.3 * Material[2].k_d;
    Material[2].k_r = vec3( 1.0, 1.0, 1.0 );
    Material[2].k_rg = 0.4 * Material[2].k_r;
    Material[2].n = 128.0;
    
    //Copper material
    Material[3].k_d = vec3( 0.780392, 0.568627, 0.113725 );
    Material[3].k_a = vec3( 0.329412, 0.223529, 0.027451 );
    Material[3].k_r = vec3( 0.992157, 0.941176, 0.807843 );
    Material[3].k_rg = 0.4 * Material[3].k_r;
    Material[3].n = 27.0;
    
    //Red diomand
    Material[4].k_d = vec3( 0.614240, 0.041360, 0.041360 );
    Material[4].k_a = vec3( 0.174500, 0.011750, 0.011750 );
    Material[4].k_r = vec3( 0.727811, 0.626959, 0.626959 );
    Material[4].k_rg = vec3( 0.550000, 0.550000, 0.550000);
    Material[4].n = 128.0;
    
    //pearl
    Material[5].k_d = vec3( 1.000000, 0.829000, 0.829000 );
    Material[5].k_a = vec3( 0.250000, 0.207250, 0.207250 );
    Material[5].k_r = vec3( 0.296648, 0.296648, 0.296648 );
    Material[5].k_rg = 0.4 * Material[5].k_r;
    Material[5].n = 128.0;
    
    //Bronze
    Material[6].k_d = vec3( 0.714000, 0.428400, 0.181440 );
    Material[6].k_a = vec3( 0.212500, 0.127500, 0.054000 );
    Material[6].k_r = vec3( 0.393548, 0.271906, 0.166721 );
    Material[6].k_rg = 0.4 * Material[6].k_r;
    Material[6].n = 128.0;
    
    //violet
    Material[7].k_d = vec3( 0.430000, 0.470000, 0.540000 );
    Material[7].k_a = vec3( 0.110000, 0.060000, 0.090000 );
    Material[7].k_r = vec3( 0.330000, 0.330000, 0.520000 );
    Material[7].k_rg = 0.4 * Material[7].k_r;
    Material[7].n = 128.0;
    
    //black
    Material[8].k_d = vec3( 0., 0., 0. );
    Material[8].k_a = vec3( 0.110000, 0.060000, 0.090000 );
    Material[8].k_r = vec3( 0.330000, 0.330000, 0.520000 );
    Material[8].k_rg = 0.4 * Material[8].k_r;
    Material[8].n = 128.0;
    
    //special
    Material[9].k_d = vec3( 0., 0., 0. );
    Material[9].k_a = vec3( 0.0, 0.0, 0.0 );
    Material[9].k_r = vec3( 0.0, 0.0, 0.0 );
    Material[9].k_rg = 0.4 * Material[9].k_r;
    Material[9].n = 128.0;
    
    // Light 0.
    Light[0].position = vec3( 10.0*abs(sin(0.2*PI*iTime)), 10.0, 10.0*abs(cos(0.2*PI*iTime)) );
    Light[0].I_a = vec3( 0.1, 0.1, 0.1 );
    Light[0].I_source = vec3( 0.8*abs(sin(0.2*PI*iTime)), 0.8*abs(cos(0.2*PI*iTime)), 0.8*abs(sin(1.2*PI*iTime)) );
    Light[0].I_source = vec3( 0.8*abs(sin(0.2*PI*iTime)), 1., 1. );

  // Light 1.
    Light[1].position = vec3( -10.0*abs(cos(0.2*PI*iTime)), 10.0, -10.0*abs(sin(0.2*PI*iTime)) );
    Light[1].I_a = vec3( 0.1, 0.1, 0.1 );
    Light[1].I_source = vec3( 0.8*abs(sin(0.2*PI*iTime)), 0.8*abs(cos(0.1*PI*iTime)), 0.8*abs(sin(1.4*PI*iTime)) );
    
/*    // Light 2
    Light[2].position = vec3( 9.9, 12.0, 9.9 );
    Light[2].I_a = vec3( 0.1, 0.1, 0.1 );
    Light[2].I_source = vec3( 0.8*abs(sin(0.2*PI*iTime)), 0.8*abs(cos(0.4*PI*iTime)), 0.8*abs(sin(1.3*PI*iTime)) );

    // Light 3.
    Light[3].position = vec3( -9.9, 12.0, 9.9 );
    Light[3].I_a = vec3( 0.1, 0.1, 0.1 );
    Light[3].I_source = vec3( 0.6*abs(sin(0.1*PI*iTime)), 0.6*abs(cos(0.3*PI*iTime)), 0.6*abs(sin(1.1*PI*iTime)) );
*/
}



/////////////////////////////////////////////////////////////////////////////
// Computes intersection between a plane and a ray.
// Returns true if there is an intersection where the ray parameter t is
// between tmin and tmax, otherwise returns false.
// If there is such an intersection, outputs the value of t, the position
// of the intersection (hitPos) and the normal vector at the intersection
// (hitNormal).
/////////////////////////////////////////////////////////////////////////////
bool IntersectPlane( in Plane_t pln, in Ray_t ray, in float tmin, in float tmax,
                     out float t, out vec3 hitPos, out vec3 hitNormal )
{
    vec3 N = vec3( pln.A, pln.B, pln.C );
    float NRd = dot( N, ray.d );
    float NRo = dot( N, ray.o );
    float t0 = (-pln.D - NRo) / NRd;
    if ( t0 < tmin || t0 > tmax ) return false;

    // We have a hit -- output results.
    t = t0;
    hitPos = ray.o + t0 * ray.d;
    hitNormal = normalize( N );
    return true;
}



/////////////////////////////////////////////////////////////////////////////
// Computes intersection between a plane and a ray.
// Returns true if there is an intersection where the ray parameter t is
// between tmin and tmax, otherwise returns false.
/////////////////////////////////////////////////////////////////////////////
bool IntersectPlane( in Plane_t pln, in Ray_t ray, in float tmin, in float tmax )
{
    vec3 N = vec3( pln.A, pln.B, pln.C );
    float NRd = dot( N, ray.d );
    float NRo = dot( N, ray.o );
    float t0 = (-pln.D - NRo) / NRd;
    if ( t0 < tmin || t0 > tmax ) return false;
    return true;
}



/////////////////////////////////////////////////////////////////////////////
// Computes intersection between a sphere and a ray.
// Returns true if there is an intersection where the ray parameter t is
// between tmin and tmax, otherwise returns false.
// If there is one or two such intersections, outputs the value of the
// smaller t, the position of the intersection (hitPos) and the normal
// vector at the intersection (hitNormal).
/////////////////////////////////////////////////////////////////////////////
bool IntersectSphere( in Sphere_t sph, in Ray_t ray, in float tmin, in float tmax,
                      out float t, out vec3 hitPos, out vec3 hitNormal )
{
    vec3 Ro = ray.o - sph.center;
    vec3 Rd = ray.d;
    
    float a = dot(Rd, Rd);
    float b = 2.0 * dot(Rd, Ro);
    float c = dot(Ro, Ro) - sph.radius * sph.radius;
    float d = b * b - 4.0 * a * c;
    
    if (d >= 0.0 && b <= 0.0)
    {
        // Computed at the nearest intersection point.
        t = (-b - sqrt(d)) / (2.0 * a);
        hitPos = ray.o + t * ray.d;
        hitNormal = normalize(hitPos - sph.center);
        return (t >= tmin && t <= tmax);
    }
    
    /////////////////////////////////
    // TASK: WRITE YOUR CODE HERE. //
    /////////////////////////////////

    return false;  // Replace this with your code.

}



/////////////////////////////////////////////////////////////////////////////
// Computes intersection between a sphere and a ray.
// Returns true if there is an intersection where the ray parameter t is
// between tmin and tmax, otherwise returns false.
/////////////////////////////////////////////////////////////////////////////
bool IntersectSphere( in Sphere_t sph, in Ray_t ray, in float tmin, in float tmax )
{
    vec3 Ro = ray.o - sph.center;
    vec3 Rd = ray.d;
    
    float a = dot(Rd, Rd);
    float b = 2.0 * dot(Rd, Ro);
    float c = dot(Ro, Ro) - sph.radius * sph.radius;
    float d = b * b - 4.0 * a * c;
    
    if (d >= 0.0 && b <= 0.0)
    {
        // Computed at the nearest intersection point.
        float t = (-b - sqrt(d)) / (2.0 * a);
        return (t >= tmin && t <= tmax);
    }
    
    /////////////////////////////////
    // TASK: WRITE YOUR CODE HERE. //
    /////////////////////////////////

    return false;  // Replace this with your code.

}



/////////////////////////////////////////////////////////////////////////////
// Computes (I_a * k_a) + k_shadow * I_source * [ k_d * (N.L) + k_r * (R.V)^n ].
// Input vectors L, N and V are pointing AWAY from surface point.
// Assume all vectors L, N and V are unit vectors.
/////////////////////////////////////////////////////////////////////////////
vec3 PhongLighting( in vec3 L, in vec3 N, in vec3 V, in bool inShadow,
                    in Material_t mat, in Light_t light )
{
    if ( inShadow ) {
        return light.I_a * mat.k_a;
    }
    else {
        vec3 R = reflect( -L, N );
        float N_dot_L = max( 0.0, dot( N, L ) );
        float R_dot_V = max( 0.0, dot( R, V ) );
        float R_dot_V_pow_n = ( R_dot_V == 0.0 )? 0.0 : pow( R_dot_V, mat.n );

        return light.I_a * mat.k_a +
               light.I_source * (mat.k_d * N_dot_L + mat.k_r * R_dot_V_pow_n);
    }
}

//refer to https://www.shadertoy.com/view/XdB3Dw
vec3 squaresColours(vec2 p)
{
	p+=vec2(iTime*0.2);
	
	vec3 orange=vec3(1.0,0.4,0.1)*2.0;
	vec3 purple=vec3(1.0,0.2,0.5)*0.8;
	
	float l=pow(0.5+0.5*cos(p.x*7.0+cos(p.y)*8.0)*sin(p.y*2.0),4.0)*2.0;
	vec3 c=pow(l*(mix(orange,purple,0.5+0.5*cos(p.x*40.0+sin(p.y*10.0)*3.0))+
				  mix(orange,purple,0.5+0.5*cos(p.x*20.0+sin(p.y*3.0)*3.0))),vec3(1.2))*0.7;
	
	c+=vec3(1.0,0.8,0.4)*pow(0.5+0.5*cos(p.x*20.0)*sin(p.y*12.0),20.0)*2.0;
	
	c+=vec3(0.1,0.5+0.5*cos(p*20.0))*vec3(0.05,0.1,0.4).bgr*0.7;
	
	return c;
}

vec3 calculateColor(vec2 p,float border){
    float sm=0.02;
	vec2 res=vec2(3.5);
	vec2 ip=floor(p*res)/res;
	vec2 fp=fract(p*res);
	float m=1.0-max(smoothstep(border-sm,border,abs(fp.x-0.5)),smoothstep(border-sm,border,abs(fp.y-0.5)));
	m+=1.0-smoothstep(0.0,0.56,distance(fp,vec2(0.5)));
	return m*squaresColours(ip);
}

vec3 PhongLighting( in vec3 L, in vec3 N, in vec3 V, in bool inShadow,
                    in Light_t light, vec3 nearest_hitPos, int hitWhichPlane )
{
    Material_t mat = Material[1];
    vec2 p, intPart;
    float countIntPart;
    if(hitWhichPlane == 0 || hitWhichPlane == 5) p = nearest_hitPos.xz;
    else if(hitWhichPlane == 1 || hitWhichPlane == 3) p = nearest_hitPos.xy;
    else if(hitWhichPlane == 2 || hitWhichPlane == 4) p = nearest_hitPos.yz;
    else if(hitWhichPlane < 0){p.y = nearest_hitPos.y; p.x = length(nearest_hitPos.xz);}
    p = p / blockNum;
    mat.k_d = calculateColor(p, blockLength);
    mat.k_a = 0.4 * mat.k_d;
    mat.k_r = 3.0 * mat.k_d;

    if ( inShadow ) {
        return light.I_a * mat.k_a;
    }
    else {
        vec3 R = reflect( -L, N );
        float N_dot_L = max( 0.0, dot( N, L ) );
        float R_dot_V = max( 0.0, dot( R, V ) );
        float R_dot_V_pow_n = ( R_dot_V == 0.0 )? 0.0 : pow( R_dot_V, mat.n );

        return light.I_a * mat.k_a +
               light.I_source * (mat.k_d * N_dot_L + mat.k_r * R_dot_V_pow_n);
    }
}

/////////////////////////////////////////////////////////////////////////////
// Casts a ray into the scene and returns color computed at the nearest
// intersection point. The color is the sum of light from all light sources,
// each computed using Phong Lighting Model, with consideration of
// whether the interesection point is being shadowed from the light.
// If there is no interesection, returns the background color, and outputs
// hasHit as false.
// If there is intersection, returns the computed color, and outputs
// hasHit as true, the 3D position of the intersection (hitPos), the
// normal vector at the intersection (hitNormal), and the k_rg value
// of the material of the intersected object.
/////////////////////////////////////////////////////////////////////////////
vec3 CastRay( in Ray_t ray,
              out bool hasHit, out vec3 hitPos, out vec3 hitNormal, out vec3 k_rg )
{
    // Find whether and where the ray hits some object.
    // Take the nearest hit point.

    bool hasHitSomething = false;
    float nearest_t = DEFAULT_TMAX;   // The ray parameter t at the nearest hit point.
    vec3 nearest_hitPos;              // 3D position of the nearest hit point.
    vec3 nearest_hitNormal;           // Normal vector at the nearest hit point.
    int nearest_hitMatID;             // MaterialID of the object at the nearest hit point.

    float temp_t;
    vec3 temp_hitPos;
    vec3 temp_hitNormal;
    bool temp_hasHit;
    
    // Which Plane does it hit
    int hitWhichPlane;
    
    /////////////////////////////////////////////////////////////////////////////
    // TASK:
    // * Try interesecting input ray with all the planes and spheres,
    //   and record the front-most (nearest) interesection.
    // * If there is interesection, need to record hasHitSomething,
    //   nearest_t, nearest_hitPos, nearest_hitNormal, nearest_hitMatID.
    /////////////////////////////////////////////////////////////////////////////
    
    // Try interesecting input ray with all the planes,
    // and record the front-most (nearest) interesection.
    for (int i = 0; i < NUM_PLANES; i++)
    {
        temp_hasHit = IntersectPlane(Plane[i], ray, DEFAULT_TMIN, DEFAULT_TMAX, temp_t, temp_hitPos, temp_hitNormal);
        
        // If hit, set hasHitSomething as true,
        // and update the infomation of front-most (nearest) interesection.
        if (temp_hasHit && temp_t < nearest_t)
        {
            hasHitSomething = true;
            nearest_t = temp_t;
            nearest_hitPos = temp_hitPos;
            nearest_hitNormal = temp_hitNormal;
            if(Plane[i].type == 1)
            {
                float tempX = mod(nearest_hitPos.x,5.)-2.5;
                float tempZ = mod(nearest_hitPos.z,5.)-2.5;
                if(tempX*tempZ<0.)
                    nearest_hitMatID = Plane[i].materialID_1;
                else
                    nearest_hitMatID = Plane[i].materialID_2;
            }
            else 
                nearest_hitMatID = Plane[i].materialID_1;
            hitWhichPlane = i;
        }
    }
    
    // Try interesecting input ray with all the spheres,
    // and record the front-most (nearest) interesection.
    for (int i = 0; i < NUM_SPHERES; i++)
    {
        temp_hasHit = IntersectSphere(Sphere[i], ray, DEFAULT_TMIN, DEFAULT_TMAX, temp_t, temp_hitPos, temp_hitNormal);
        
        // If hit, set hasHitSomething as true,
        // and update the infomation of front-most (nearest) interesection.
        if (temp_hasHit && temp_t < nearest_t)
        {
            hasHitSomething = true;
            nearest_t = temp_t;
            nearest_hitPos = temp_hitPos;
            nearest_hitNormal = temp_hitNormal;
            nearest_hitMatID = Sphere[i].materialID;
        }
    }
    
    /////////////////////////////////
    // TASK: WRITE YOUR CODE HERE. //
    /////////////////////////////////



    // One of the output results.
    hasHit = hasHitSomething;
    if ( !hasHitSomething ) return BACKGROUND_COLOR;

    vec3 I_local = vec3( 0.0 );  // Result color will be accumulated here.

    /////////////////////////////////////////////////////////////////////////////
    // TASK:
    // * Accumulate lighting from each light source on the nearest hit point.
    //   They are all accumulated into I_local.
    // * For each light source, make a shadow ray, and check if the shadow ray
    //   intersects any of the objects (the planes and spheres) between the
    //   nearest hit point and the light source.
    // * Then, call PhongLighting() to compute lighting for this light source.
    /////////////////////////////////////////////////////////////////////////////

    Ray_t ShadowRay;
    bool Shadow;
    
    // Accumulate lighting from each light source on the nearest hit point.
    // They are all accumulated into I_local.
    for (int i = 0; i < NUM_LIGHTS; i++)
    {
        // Init the boolean variable Shadow as false.
        Shadow = false;
        
        // For each light source, make a shadow ray.
        ShadowRay.o = nearest_hitPos;
        ShadowRay.d = normalize(Light[i].position - ShadowRay.o);
        
        // Check if the shadow ray
        // intersects any of the planes between the
        // nearest hit point and the light source.
        for (int j = 0; !Shadow && j < NUM_PLANES; j++)
            Shadow = IntersectPlane(Plane[j], ShadowRay, DEFAULT_TMIN, distance(nearest_hitPos, Light[i].position));
        
        // Check if the shadow ray
        // intersects any of the spheres between the
        // nearest hit point and the light source.
        for (int j = 0; !Shadow && j < NUM_SPHERES; j++)
            Shadow = IntersectSphere(Sphere[j], ShadowRay, DEFAULT_TMIN, distance(nearest_hitPos, Light[i].position));
        
        // Then, call PhongLighting() to compute lighting for this light source.
        I_local += PhongLighting(ShadowRay.d, nearest_hitNormal, -ray.d, Shadow,
                   Material[nearest_hitMatID], Light[i]);
                   
        if(nearest_hitMatID == DISCO_COLOR)
            I_local += PhongLighting(ShadowRay.d, nearest_hitNormal, -ray.d, Shadow, Light[i], nearest_hitPos, hitWhichPlane);
        else
            I_local += PhongLighting(ShadowRay.d, nearest_hitNormal, -ray.d, Shadow,
                   Material[nearest_hitMatID], Light[i]);
    }
    
    
    
    /////////////////////////////////
    // TASK: WRITE YOUR CODE HERE. //
    /////////////////////////////////



    // Populate output results.
    hitPos = nearest_hitPos;
    hitNormal = nearest_hitNormal;
    k_rg = Material[nearest_hitMatID].k_rg;

    return I_local;
}



/////////////////////////////////////////////////////////////////////////////
// Execution of fragment shader starts here.
// 1. Initializes the scene.
// 2. Compute a primary ray for the current pixel (fragment).
// 3. Trace ray into the scene with NUM_ITERATIONS recursion levels.
/////////////////////////////////////////////////////////////////////////////
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    InitScene();

    // Scale pixel 2D position such that its y coordinate is in [-1.0, 1.0].
    vec2 pixel_pos = (2.0 * fragCoord.xy - iResolution.xy) / iResolution.y;

    vec2 m = iMouse.xy/iResolution.xy;
    float an = 0.3*iTime - 7.0*m.x;

    // Position the camera.
    vec3 cam_pos = vec3( 3.5*sin(an), 10.0, 3.5*cos(an)+8.0 );
    vec3 cam_lookat = vec3( 0, 4.0, -1.0 );
    vec3 cam_up_vec = vec3( 0.0, 1.0, 0.0 );

    // Set up camera coordinate frame in world space.
    vec3 cam_z_axis = normalize( cam_pos - cam_lookat );
    vec3 cam_x_axis = normalize( cross(cam_up_vec, cam_z_axis) );
    vec3 cam_y_axis = normalize( cross(cam_z_axis, cam_x_axis));

    // Create primary ray.
    float pixel_pos_z = -1.0 / tan(FOVY / 2.0);
    Ray_t pRay;
    pRay.o = cam_pos;
    pRay.d = normalize( pixel_pos.x * cam_x_axis  +  pixel_pos.y * cam_y_axis  +  pixel_pos_z * cam_z_axis );


    // Start Ray Tracing.
    // Use iterations to emulate the recursion.

    vec3 I_result = vec3( 0.0 );
    vec3 compounded_k_rg = vec3( 1.0 );
    Ray_t nextRay = pRay;

    for ( int level = 0; level <= NUM_ITERATIONS; level++ )
    {
        bool hasHit;
        vec3 hitPos, hitNormal, k_rg;

        vec3 I_local = CastRay( nextRay, hasHit, hitPos, hitNormal, k_rg );

        I_result += compounded_k_rg * I_local;

        if ( !hasHit ) break;

        compounded_k_rg *= k_rg;

        nextRay = Ray_t( hitPos, normalize( reflect(nextRay.d, hitNormal) ) );
    }

    fragColor = vec4( I_result, 1.0 );
}
