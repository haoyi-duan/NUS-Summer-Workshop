//============================================================
// STUDENT NAME: Duan Haoyi
// NUS User ID.: t0925750
// COMMENTS TO GRADER: 
//
// ============================================================

// APPLICATION PROGRAM

#include <cstdlib>
#include <cstdio>
using namespace std;

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>
#else
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#endif

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "helper/trackball.h"
#include "helper/glslprogram.h"
#include "helper/vbocube.h"
#include "helper/vboteapot.h"
#include "helper/vboplane.h"
#include "helper/vbotorus.h"


// Shaders' filenames.
const char vertShaderFile[] = "shader.vert";
const char fragShaderFile[] = "shader.frag";

GLSLProgram shaderProg;  // Contains the shader program object.


// Light info. Must be a point light.
glm::vec3 lightPosition;                                     // In world space.
const glm::vec3 lightLookAt = glm::vec3(0.0f, 1.5f, 0.0f);   // In world space.
const glm::vec3 lightUpVector = glm::vec3(0.0f, 1.0f, 0.0f); // In world space.
const float lightVerticalFOV = 30.0f;                        // Vertical FOV in degrees.

const glm::vec3 lightAmbient  = glm::vec3(0.2f, 0.2f, 0.2f);
//const glm::vec3 lightDiffuse  = glm::vec3(1.0f, 1.0f, 1.0f); // Not used.
//const glm::vec3 lightSpecular = glm::vec3(1.0f, 1.0f, 1.0f); // Not used.


// 3D object models.
const int numObjects = 4;
Drawable *objModel[numObjects];


// For FBO.
const int fboWidth = 640;
const int fboHeight = 480;
GLuint fboHandle;


// For Shadowmap.
const int shadowMapWidth = fboWidth;
const int shadowMapHeight = fboHeight;

glm::mat4 shadowMatrix;  // Transforms point in world coordinates to shadowmap coordinates.


// For window and viewport size.
int winWidth = 800;     // Window width in pixels.
int winHeight = 600;    // Window height in pixels.

bool wireframeMode = false;


// For orbiting the light position around its "look-at" point.
// Use keyboard to move light position.
const float init_lightDist = 11.0f;       // Distance from ligth's "look-at" point.
const float init_lightLatitude = 18.5f;   // In degrees.
const float init_lightLongitude = 197.0f; // In degrees.
const float lightDirectionIncr = 2.0f;    // In degrees.
const float lightDistanceIncr = 0.5f;

float lightDist;
float lightLatitude;
float lightLongitude;


// For trackball.
double prevMouseX, prevMouseY;
bool mouseLeftPressed;
bool mouseMiddlePressed;
bool mouseRightPressed;
float cam_curr_quat[4];
float cam_prev_quat[4];
float cam_eye[3], cam_lookat[3], cam_up[3];
const float initial_cam_eye[3] = { 0.0f, 5.0f, 8.0f };     // World coordinates.
const float initial_cam_lookat[3] = { 0.0f, 0.0f, 0.0f };  // World coordinates.



/////////////////////////////////////////////////////////////////////////////
// Set up texture maps from image files.
/////////////////////////////////////////////////////////////////////////////
static void SetUpTextureMapsFromFiles(void)
{
    const int numTexMaps = 1;
    const GLenum texUnit[numTexMaps] = { GL_TEXTURE1 };
    const char *texMapFile[numTexMaps] = { "images/flower2.jpg" };

    // Enable fliping of images vertically when read in.
    // This is to follow OpenGL's image coordinate system, i.e. bottom-leftmost is (0, 0).
    stbi_set_flip_vertically_on_load(true);

    // Read texture images from files.
    for (int t = 0; t < numTexMaps; t++) {

        int imgWidth, imgHeight, numComponents;

        GLubyte *imgData = stbi_load(texMapFile[t], &imgWidth, &imgHeight, &numComponents, 0);

        if (imgData == NULL) {
            fprintf(stderr, "Error: Fail to read image file %s.\n", texMapFile[t]);
            exit(EXIT_FAILURE);
        }
        printf("%s (%d x %d, %d components)\n", texMapFile[t], imgWidth, imgHeight, numComponents);

        glActiveTexture(texUnit[t]);
        GLuint tid;
        glGenTextures(1, &tid);
        glBindTexture(GL_TEXTURE_2D, tid);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

        if (numComponents == 1) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, imgWidth, imgHeight, 0,
                         GL_RED, GL_UNSIGNED_BYTE, imgData);
        }
        else if (numComponents == 3) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, imgWidth, imgHeight, 0,
                GL_RGB, GL_UNSIGNED_BYTE, imgData);
        }
        else {
            fprintf(stderr, "Error: Unexpected image format.\n");
            exit(EXIT_FAILURE);
        }

        stbi_image_free(imgData);

        glGenerateMipmap(GL_TEXTURE_2D);
    }
}



/////////////////////////////////////////////////////////////////////////////
// Set up FBO and attach shadowmap to it.
/////////////////////////////////////////////////////////////////////////////
static void SetUpShadowMapAndFBO(void)
{
    const GLenum texUnit = GL_TEXTURE0;
    const GLint texInternalFormat = GL_DEPTH_COMPONENT32F;
    const GLenum attachPoint = GL_DEPTH_ATTACHMENT;

    /////////////////////////////////////////////////////////////////////////////
    // TASK 2:
    // Set up a texture object for the shadowmap, and attach it to a FBO.
    // Refer to the lecture slides.
    /////////////////////////////////////////////////////////////////////////////

    const GLfloat texBorder[] = { 0.5f, 0.0f, 0.0f, 0.0f }; // Fill in correct border values.
	
	// The shadwomap texture 
	glActiveTexture(texUnit); // Bind shadow map to texture nuit 0
	GLuint depthTex;
	glGenTextures(1, &depthTex);
	glBindTexture(GL_TEXTURE_2D, depthTex);
	glTexImage2D(GL_TEXTURE_2D, 0, texInternalFormat,
		shadowMapWidth, shadowMapHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, texBorder);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE,
		GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	// Create and set up the FBO
	glGenFramebuffers(1, &fboHandle);
	glBindFramebuffer(GL_FRAMEBUFFER, fboHandle);
	// Attach this texture to FBO
	glFramebufferTexture2D(GL_FRAMEBUFFER, attachPoint, GL_TEXTURE_2D, depthTex, 0);
	GLenum drawBuffers[] = { GL_NONE };
	glDrawBuffers(1, drawBuffers);
	// Revert to the default framebuffer for now
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	///////////////////////////////////
    // TASK 2: WRITE YOUR CODE HERE. //
    ///////////////////////////////////

}



/////////////////////////////////////////////////////////////////////////////
// Compute the light source 3D position from its direction and distance
// from its "look-at" point
/////////////////////////////////////////////////////////////////////////////
static void ComputeLightPosition(void)
{
    glm::mat4 xformMat = glm::mat4(1.0f);
    xformMat = glm::translate(xformMat, lightLookAt);
    xformMat = glm::rotate(xformMat, glm::radians(lightLongitude), glm::vec3(0.0f, 1.0f, 0.0f));
    xformMat = glm::rotate(xformMat, glm::radians(lightLatitude), glm::vec3(0.0f, 0.0f, 1.0f));
    lightPosition = xformMat * glm::vec4(lightDist, 0.0f, 0.0f, 1.0f);
}



/////////////////////////////////////////////////////////////////////////////
// The init function. It initializes some OpenGL states.
/////////////////////////////////////////////////////////////////////////////
static void MyInit(void)
{
    try {
        shaderProg.compileShader(vertShaderFile, GLSLShader::VERTEX);
        shaderProg.compileShader(fragShaderFile, GLSLShader::FRAGMENT);
        shaderProg.link();
        //shaderProg.validate();
        shaderProg.use(); // Install shader program to rendering pipeline.
    }
    catch(GLSLProgramException &e) {
        fprintf(stderr, "Error: %s.\n", e.what());
        exit(EXIT_FAILURE);
    }

    // Create geometry of 3D object models.
    objModel[0] = new VBOPlane(1.0, 1.0, 32, 32);
    objModel[1] = new VBOCube;
    objModel[2] = new VBOTorus(0.7, 0.3, 32, 64);
    objModel[3] = new VBOTeapot(10, glm::mat4(1.0f));

    // Set up texture maps from image files.
    SetUpTextureMapsFromFiles();

    // Set up FBO and attach shadow map to it.
    SetUpShadowMapAndFBO();


    // Initialize some OpenGL states.
    glClearColor(0.0, 0.0, 0.0, 1.0); // Set background color to space black.
    glEnable(GL_DEPTH_TEST); // Use depth-buffer for hidden surface removal.

    // Initialize light position.
    lightDist = init_lightDist;
    lightLatitude = init_lightLatitude;
    lightLongitude = init_lightLongitude;
    ComputeLightPosition();

    // Initialization for trackball.
    trackball(cam_curr_quat, 0, 0, 0, 0);
    cam_eye[0] = initial_cam_eye[0];
    cam_eye[1] = initial_cam_eye[1];
    cam_eye[2] = initial_cam_eye[2];
    cam_lookat[0] = initial_cam_lookat[0];
    cam_lookat[1] = initial_cam_lookat[1];
    cam_lookat[2] = initial_cam_lookat[2];
    cam_up[0] = 0.0f;
    cam_up[1] = 1.0f;
    cam_up[2] = 0.0f;
}



/////////////////////////////////////////////////////////////////////////////
// Draw the 3D objects.
/////////////////////////////////////////////////////////////////////////////
static void RenderObjects( const glm::mat4 &viewMat, const glm::mat4 &projMat )
{
    {   // Draw the floor (-y) plane.
        glm::mat4 modelMat = glm::mat4(1.0f);
        modelMat = glm::rotate(modelMat, glm::radians(-15.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMat = glm::translate( modelMat, glm::vec3(0.0f, 0.0f, 0.0f) );
        modelMat = glm::scale( modelMat, glm::vec3(8.0f, 8.0f, 8.0f) );

        glm::mat4 modelViewMat = viewMat * modelMat;
        glm::mat4 modelViewProjMat = projMat * modelViewMat;
        glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(modelViewMat)));

        shaderProg.setUniform("ModelMatrix", modelMat);
        shaderProg.setUniform("ModelViewMatrix", modelViewMat);
        shaderProg.setUniform("ModelViewProjMatrix", modelViewProjMat);
        shaderProg.setUniform("NormalMatrix", normalMat);

        shaderProg.setUniform("MatlAmbient", glm::vec3(0.75f, 0.75f, 1.0f) );
        shaderProg.setUniform("MatlDiffuse", glm::vec3(0.75f, 0.75f, 1.0f) );
        shaderProg.setUniform("MatlSpecular", glm::vec3(0.8f, 0.8f, 0.8f));
        shaderProg.setUniform("MatlShininess", 8.0f );

        objModel[0]->render();
    }

    {   // Draw the right (+x) plane.
        glm::mat4 modelMat = glm::mat4(1.0f);
        modelMat = glm::rotate(modelMat, glm::radians(-15.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMat = glm::translate(modelMat, glm::vec3(4.0f, 4.0f, 0.0f));
        modelMat = glm::rotate(modelMat, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        modelMat = glm::scale(modelMat, glm::vec3(8.0f, 8.0f, 8.0f));

        glm::mat4 modelViewMat = viewMat * modelMat;
        glm::mat4 modelViewProjMat = projMat * modelViewMat;
        glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(modelViewMat)));

        shaderProg.setUniform("ModelMatrix", modelMat);
        shaderProg.setUniform("ModelViewMatrix", modelViewMat);
        shaderProg.setUniform("ModelViewProjMatrix", modelViewProjMat);
        shaderProg.setUniform("NormalMatrix", normalMat);

        shaderProg.setUniform("MatlAmbient", glm::vec3(0.75f, 0.75f, 1.0f));
        shaderProg.setUniform("MatlDiffuse", glm::vec3(0.75f, 0.75f, 1.0f));
        shaderProg.setUniform("MatlSpecular", glm::vec3(0.8f, 0.8f, 0.8f));
        shaderProg.setUniform("MatlShininess", 8.0f);

        objModel[0]->render();
    }

    {   // Draw the back (-z) plane.
        glm::mat4 modelMat = glm::mat4(1.0f);
        modelMat = glm::rotate(modelMat, glm::radians(-15.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMat = glm::translate(modelMat, glm::vec3(0.0f, 4.0f, -4.0f));
        modelMat = glm::rotate(modelMat, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMat = glm::scale(modelMat, glm::vec3(8.0f, 8.0f, 8.0f));

        glm::mat4 modelViewMat = viewMat * modelMat;
        glm::mat4 modelViewProjMat = projMat * modelViewMat;
        glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(modelViewMat)));

        shaderProg.setUniform("ModelMatrix", modelMat);
        shaderProg.setUniform("ModelViewMatrix", modelViewMat);
        shaderProg.setUniform("ModelViewProjMatrix", modelViewProjMat);
        shaderProg.setUniform("NormalMatrix", normalMat);

        shaderProg.setUniform("MatlAmbient", glm::vec3(0.75f, 0.75f, 1.0f));
        shaderProg.setUniform("MatlDiffuse", glm::vec3(0.75f, 0.75f, 1.0f));
        shaderProg.setUniform("MatlSpecular", glm::vec3(0.8f, 0.8f, 0.8f));
        shaderProg.setUniform("MatlShininess", 8.0f);

        objModel[0]->render();
    }

    {   // Draw the box.
        glm::mat4 modelMat = glm::mat4(1.0f);
        modelMat = glm::translate(modelMat, glm::vec3(0.8f, 0.0f, -0.8f));
        modelMat = glm::rotate(modelMat, glm::radians(-45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMat = glm::scale(modelMat, glm::vec3(2.0f, 3.0f, 1.0f));
        modelMat = glm::translate(modelMat, glm::vec3(0.0f, 0.5f, 0.0f));

        glm::mat4 modelViewMat = viewMat * modelMat;
        glm::mat4 modelViewProjMat = projMat * modelViewMat;
        glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(modelViewMat)));

        shaderProg.setUniform("ModelMatrix", modelMat);
        shaderProg.setUniform("ModelViewMatrix", modelViewMat);
        shaderProg.setUniform("ModelViewProjMatrix", modelViewProjMat);
        shaderProg.setUniform("NormalMatrix", normalMat);

        shaderProg.setUniform("MatlAmbient", glm::vec3(1.0f, 0.5f, 0.5f));
        shaderProg.setUniform("MatlDiffuse", glm::vec3(1.0f, 0.5f, 0.5f));
        shaderProg.setUniform("MatlSpecular", glm::vec3(0.5f, 0.5f, 0.5f));
        shaderProg.setUniform("MatlShininess", 8.0f);

        objModel[1]->render();
    }

    {   // Draw the torus.
        glm::mat4 modelMat = glm::mat4(1.0f);
        modelMat = glm::translate(modelMat, glm::vec3(-2.0f, 1.5f, -1.0f));
        modelMat = glm::scale(modelMat, glm::vec3(1.5f, 1.5f, 1.5f));
        modelMat = glm::rotate(modelMat, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        glm::mat4 modelViewMat = viewMat * modelMat;
        glm::mat4 modelViewProjMat = projMat * modelViewMat;
        glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(modelViewMat)));

        shaderProg.setUniform("ModelMatrix", modelMat);
        shaderProg.setUniform("ModelViewMatrix", modelViewMat);
        shaderProg.setUniform("ModelViewProjMatrix", modelViewProjMat);
        shaderProg.setUniform("NormalMatrix", normalMat);

        shaderProg.setUniform("MatlAmbient", glm::vec3(0.5f, 1.0f, 0.5f));
        shaderProg.setUniform("MatlDiffuse", glm::vec3(0.5f, 1.0f, 0.5f));
        shaderProg.setUniform("MatlSpecular", glm::vec3(0.5f, 0.5f, 0.5f));
        shaderProg.setUniform("MatlShininess", 8.0f);

        objModel[2]->render();
    }

    {   // Draw the teapot.
        glm::mat4 modelMat = glm::mat4(1.0f);
        modelMat = glm::translate(modelMat, glm::vec3(-0.5f, 0.0f, 2.0f));
        modelMat = glm::scale(modelMat, glm::vec3(0.8f, 0.8f, 0.8f));
        modelMat = glm::rotate(modelMat, glm::radians(150.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMat = glm::rotate(modelMat, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

        glm::mat4 modelViewMat = viewMat * modelMat;
        glm::mat4 modelViewProjMat = projMat * modelViewMat;
        glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(modelViewMat)));

        shaderProg.setUniform("ModelMatrix", modelMat);
        shaderProg.setUniform("ModelViewMatrix", modelViewMat);
        shaderProg.setUniform("ModelViewProjMatrix", modelViewProjMat);
        shaderProg.setUniform("NormalMatrix", normalMat);

        shaderProg.setUniform("MatlAmbient", glm::vec3(0.5f, 0.5f, 1.0f));
        shaderProg.setUniform("MatlDiffuse", glm::vec3(0.5f, 0.5f, 1.0f));
        shaderProg.setUniform("MatlSpecular", glm::vec3(1.0f, 1.0f, 1.0f));
        shaderProg.setUniform("MatlShininess", 32.0f);

        objModel[3]->render();
    }
}



/////////////////////////////////////////////////////////////////////////////
// Draw the shadowmap.
/////////////////////////////////////////////////////////////////////////////
static void RenderShadowMap(void)
{
    // Bind to shadowmap's FBO.
    glBindFramebuffer(GL_FRAMEBUFFER, fboHandle);
    glViewport(0, 0, fboWidth, fboHeight); // Viewport for the texture.
    glClear(GL_DEPTH_BUFFER_BIT);

    // Perspective projection matrix.
    glm::mat4 projMat = glm::perspective(glm::radians(lightVerticalFOV),
                                         (float)fboWidth / fboHeight, 2.0f, 30.0f);
    // View transformation matrix.
    glm::mat4 viewMat = glm::lookAt(lightPosition, lightLookAt, lightUpVector);

    shaderProg.setUniform("RenderShadowMapMode", true);

    /////////////////////////////////////////////////////////////////////////////
    // TASK 3:
    // * Render the scene into the shadowmap.
    // * Compute shadowMatrix.
    /////////////////////////////////////////////////////////////////////////////

	// Prevent shadow acnes. 
	// It is the easier way to prevent shadow acnes. But may cause some minor issues.
	// Not used in the assignment, already done in shader.frag.

	//glEnable(GL_POLYGON_OFFSET_FILL); 
	//glPolygonOffset(5.0f, 5.0f);

	// Render the scene into the shadowmap.
	RenderObjects(viewMat, projMat);
	
	// Compute shadowMatrix.
	// Transform to [0, 1] range
	glm::mat4 temp = glm::mat4(vec4(0.5, 0.0, 0.0, 0.0), vec4(0.0, 0.5, 0.0, 0.0), vec4(0.0, 0.0, 0.5, 0.0), vec4(0.5, 0.5, 0.5, 1));
	shadowMatrix = temp * projMat * viewMat;

    ///////////////////////////////////
    // TASK 3: WRITE YOUR CODE HERE. //
    ///////////////////////////////////


    shaderProg.setUniform("ShadowMatrix", shadowMatrix);
}



/////////////////////////////////////////////////////////////////////////////
// The draw function.
/////////////////////////////////////////////////////////////////////////////
static void MyDrawFunc(void)
{
    if (wireframeMode)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    //=======================================================================
    // Render shadowmap to FBO.
    //=======================================================================
    RenderShadowMap();

    //=======================================================================
    // Render to default framebuffer.
    //=======================================================================

    // Unbind texture's FBO (back to default FB).
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, winWidth, winHeight); // Viewport for main window.

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //=======================================================================
    // Set up View and Projection matrices, and draw the scene with shadows.
    //=======================================================================

    // Perspective projection matrix.
    glm::mat4 projMat = glm::perspective(glm::radians(60.0f), (float)winWidth / winHeight, 0.5f, 100.0f);

    // View transformation matrix.
    glm::mat4 viewMat = glm::lookAt(glm::vec3(cam_eye[0], cam_eye[1], cam_eye[2]),
                                    glm::vec3(cam_lookat[0], cam_lookat[1], cam_lookat[2]),
                                    glm::vec3(cam_up[0], cam_up[1], cam_up[2]));

    // Get the additional camera rotation matrix introduced by trackball.
    GLfloat rot[4][4];
    build_rotmatrix(rot, cam_curr_quat);
    glm::mat4 camRotMat = glm::mat4(rot[0][0], rot[0][1], rot[0][2], rot[0][3],
        rot[1][0], rot[1][1], rot[1][2], rot[1][3],
        rot[2][0], rot[2][1], rot[2][2], rot[2][3],
        rot[3][0], rot[3][1], rot[3][2], rot[3][3]);

    // The final view transformation has the additional rotation from trackball.
    viewMat = viewMat * camRotMat;

    glm::vec3 ecLightPosition = glm::vec3( viewMat * glm::vec4(lightPosition, 1.0f) );

    shaderProg.setUniform("RenderShadowMapMode", false);

    shaderProg.setUniform("ShadowMap", 0);
    shaderProg.setUniform("ProjectorImage", 1);

    shaderProg.setUniform("LightPosition", ecLightPosition);
    shaderProg.setUniform("LightAmbient", lightAmbient);
    //shaderProg.setUniform("LightDiffuse", lightDiffuse);  // Not used.
    //shaderProg.setUniform("LightSpecular", lightSpecular);  // Not used.

    RenderObjects( viewMat, projMat );
}



/////////////////////////////////////////////////////////////////////////////
// The reshape callback function.
/////////////////////////////////////////////////////////////////////////////
static void MyReshapeFunc(GLFWwindow *window, int w, int h)
{
    winWidth = w;
    winHeight = h;
    // glViewport(0, 0, w, h);
}



/////////////////////////////////////////////////////////////////////////////
// The keyboard callback function.
/////////////////////////////////////////////////////////////////////////////
static void MyKeyboardFunc(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {

        // Close window
        if (key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE) {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }
        else if (key == GLFW_KEY_W) {
            wireframeMode = !wireframeMode;
        }

        else if (key == GLFW_KEY_R) {
            // Reset light position.
            lightDist = init_lightDist;
            lightLatitude = init_lightLatitude;
            lightLongitude = init_lightLongitude;
            ComputeLightPosition();

            // Reset the trackball.
            trackball(cam_curr_quat, 0, 0, 0, 0);
            cam_eye[0] = initial_cam_eye[0];
            cam_eye[1] = initial_cam_eye[1];
            cam_eye[2] = initial_cam_eye[2];
            cam_lookat[0] = initial_cam_lookat[0];
            cam_lookat[1] = initial_cam_lookat[1];
            cam_lookat[2] = initial_cam_lookat[2];
            cam_up[0] = 0.0f;
            cam_up[1] = 1.0f;
            cam_up[2] = 0.0f;
        }

        // Move the light.
        else if (key == GLFW_KEY_UP && mods == GLFW_MOD_SHIFT) {
            lightDist -= lightDistanceIncr;
            if (lightDist < 1.0f) lightDist = 1.0f;
            ComputeLightPosition();
        }
        else if (key == GLFW_KEY_DOWN && mods == GLFW_MOD_SHIFT) {
            lightDist += lightDistanceIncr;
            ComputeLightPosition();
        }
        else if (key == GLFW_KEY_UP) {
            lightLatitude += lightDirectionIncr;
            if (lightLatitude > 85.0f) lightLatitude = 85.0f;
            ComputeLightPosition();
        }
        else if (key == GLFW_KEY_DOWN) {
            lightLatitude -= lightDirectionIncr;
            if (lightLatitude < -85.0f) lightLatitude = -85.0f;
            ComputeLightPosition();
        }
        else if (key == GLFW_KEY_LEFT) {
            lightLongitude -= lightDirectionIncr;
            if (lightLongitude < -360.0f) lightLongitude += 360.0f;
            ComputeLightPosition();
        }
        else if (key == GLFW_KEY_RIGHT) {
            lightLongitude += lightDirectionIncr;
            if (lightLongitude > 360.0f) lightLongitude -= 360.0f;
            ComputeLightPosition();
        }
    }
}



/////////////////////////////////////////////////////////////////////////////
// The mouse-click callback function.
/////////////////////////////////////////////////////////////////////////////
static void MyMouseClickFunc(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            mouseLeftPressed = true;
            trackball(cam_prev_quat, 0.0, 0.0, 0.0, 0.0);
        }
        else if (action == GLFW_RELEASE) {
            mouseLeftPressed = false;
        }
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            mouseRightPressed = true;
        }
        else if (action == GLFW_RELEASE) {
            mouseRightPressed = false;
        }
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        if (action == GLFW_PRESS) {
            mouseMiddlePressed = true;
        }
        else if (action == GLFW_RELEASE) {
            mouseMiddlePressed = false;
        }
    }
}



/////////////////////////////////////////////////////////////////////////////
// The mouse motion callback function.
/////////////////////////////////////////////////////////////////////////////
static void MyMouseMotionFunc(GLFWwindow* window, double mouse_x, double mouse_y)
{
    float rotScale = 1.0f;
    float transScale = 2.0f;

    if (mouseLeftPressed) {
        trackball(cam_prev_quat, rotScale * (2.0f * prevMouseX - winWidth) / (float)winWidth,
            rotScale * (winHeight - 2.0f * prevMouseY) / (float)winHeight,
            rotScale * (2.0f * mouse_x - winWidth) / (float)winWidth,
            rotScale * (winHeight - 2.0f * mouse_y) / (float)winHeight);

        add_quats(cam_prev_quat, cam_curr_quat, cam_curr_quat);
    }
    else if (mouseMiddlePressed) {
        cam_eye[0] -= transScale * (mouse_x - prevMouseX) / (float)winWidth;
        cam_lookat[0] -= transScale * (mouse_x - prevMouseX) / (float)winWidth;
        cam_eye[1] += transScale * (mouse_y - prevMouseY) / (float)winHeight;
        cam_lookat[1] += transScale * (mouse_y - prevMouseY) / (float)winHeight;
    }
    else if (mouseRightPressed) {
        cam_eye[2] += transScale * (mouse_y - prevMouseY) / (float)winHeight;
        cam_lookat[2] += transScale * (mouse_y - prevMouseY) / (float)winHeight;
    }

    // Update mouse point
    prevMouseX = mouse_x;
    prevMouseY = mouse_y;
}



static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}



static void WaitForEnterKeyBeforeExit(void)
{
    fflush(stdin);
    getchar();
}



/////////////////////////////////////////////////////////////////////////////
// The main function.
/////////////////////////////////////////////////////////////////////////////
int main( int argc, char** argv )
{
    atexit(WaitForEnterKeyBeforeExit); // std::atexit() is declared in cstdlib

    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow *window = glfwCreateWindow(winWidth, winHeight, "main", NULL, NULL);
    glfwGetFramebufferSize(window, &winWidth, &winHeight); // Required for macOS.

    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Register callback functions.
    glfwSetFramebufferSizeCallback(window, MyReshapeFunc);
    glfwSetKeyCallback(window, MyKeyboardFunc);
    glfwSetMouseButtonCallback(window, MyMouseClickFunc);
    glfwSetCursorPosCallback(window, MyMouseMotionFunc);

#ifndef __APPLE__
    // Initialize GLEW.
    GLenum err = glewInit();

    if ( err != GLEW_OK )
    {
        fprintf( stderr, "Error: %s.\n", glewGetErrorString( err ) );
        exit(EXIT_FAILURE);
    }

    printf( "Using GLEW %s.\n", glewGetString( GLEW_VERSION ) );

    //if ( !GLEW_VERSION_3_3 ) {
    //    fprintf( stderr, "Error: OpenGL 3.3 is not supported.\n" );
    //    exit(EXIT_FAILURE);
    //}
#endif

    printf( "System supports OpenGL %s.\n", glGetString(GL_VERSION) );

    MyInit();

    while (!glfwWindowShouldClose(window))
    {
        //glfwPollEvents();  // Use this if there is continuous animation.
        glfwWaitEvents();  // Use this if there is no animation.
        MyDrawFunc();
        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
