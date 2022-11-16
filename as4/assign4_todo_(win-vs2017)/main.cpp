// APPLICATION PROGRAM

#include <cstdlib>
#include <cstdio>
#include <cmath>
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


//============================================================================
// For normal rendering of 3D scene.
//============================================================================

// Shader program.
GLSLProgram drawObjectShaderProg;

// 3D object models.
const int numObjects = 4;
Drawable *objModel[numObjects];
GLuint objTexID[numObjects];  // All in GL_TEXTURE0.


// Light info. Must be a point light.
const glm::vec3 lightPosition = glm::vec3(-15.0f, 8.0f, 15.0f ); // Given in world space.
const glm::vec3 lightAmbient  = glm::vec3(0.2f, 0.2f, 0.2f);
const glm::vec3 lightDiffuse  = glm::vec3(1.0f, 1.0f, 1.0f);
const glm::vec3 lightSpecular = glm::vec3(3.0f, 3.0f, 3.0f);     // Note the high values.


// For rendering window and viewport size.
int winWidth = 800;     // Window width in pixels.
int winHeight = 600;    // Window height in pixels.



//============================================================================
// For bloom effect.
//============================================================================

// Shader program.
GLSLProgram imageBloomShaderProg;

// VBO for the full-screen quad.
GLuint fullScreenQuadVBO;


// FBO1.
// Always the same size as the window.
int fbo1Width = winWidth;
int fbo1Height = winHeight;
GLuint fbo1Handle;

// FBO2.
// Height is fixed, and width is adjusted to give same aspect ratio as window.
int fbo2Height = 128;
int fbo2Width = (int)( fbo2Height * ((double)winWidth / (double)winHeight) );
GLuint fbo2Handle;


// Texture maps for bloom effect processing.
GLuint originalImageTexID;  // In GL_TEXTURE1. Attached to FBO1, GL_COLOR_ATTACHMENT0.
GLuint thresholdImageTexID; // In GL_TEXTURE2. Attached to FBO2, GL_COLOR_ATTACHMENT0.
GLuint horizBlurImageTexID; // In GL_TEXTURE3. Attached to FBO2, GL_COLOR_ATTACHMENT1.
GLuint vertBlurImageTexID;  // In GL_TEXTURE4. Attached to FBO2, GL_COLOR_ATTACHMENT2.

GLuint depthRenderBufID;    // Renderbuffer depth buffer. Attached to FBO1, GL_DEPTH_ATTACHMENT.


// The 1D gaussian filter is supplied to fragment shader in a 1D texture map.
// Filter values are stored in the RED channel of the texture map.
const int blurFilterWidth = 51;  // Always a odd integer.
GLuint blurFilterTexID;          // In GL_TEXTURE0.


// The luminance value used to threshold the original rendered image.
const float luminanceThreshold = 0.8;


//============================================================================
// For other miscellaneous states.
//============================================================================

// Toggle rendering effects.
bool wireframeMode = false;
bool hasBloomEffect = true;

// For trackball.
double prevMouseX, prevMouseY;
bool mouseLeftPressed;
bool mouseMiddlePressed;
bool mouseRightPressed;
float cam_curr_quat[4];
float cam_prev_quat[4];
float cam_eye[3], cam_lookat[3], cam_up[3];
const float initial_cam_eye[3] = { 0.0f, 4.0f, 6.0f };  // World coordinates.
const float initial_cam_lookat[3] = { 0.0f, 0.0f, 0.0f };  // World coordinates.


//============================================================================
// Forward function declarations.
//============================================================================
static void MyDrawFunc(void);
static void RenderObjects(const glm::mat4 &viewMat, const glm::mat4 &projMat);
static void SetUpTextureMapFromFile(const char *texMapFile, bool mipmap,
                                    GLuint *texObjID = NULL, int *numColorChannels = NULL);
static void SetUpObjects();
static void ResizeFBOs();
static void SetUpFBOs();
static float gauss(float x, float sigma2);
static void SetUpBlurFilterTexture();
static void MyInit();
static void makeFullScreenQuad();



/////////////////////////////////////////////////////////////////////////////
// The draw function.
/////////////////////////////////////////////////////////////////////////////
static void MyDrawFunc(void)
{
    // Normal Rendering of 3D scene.
    {
        drawObjectShaderProg.use(); // Install shader program to rendering pipeline.

        if (wireframeMode)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        glEnable(GL_DEPTH_TEST);  // Need to use depth testing.


        if (hasBloomEffect) {
            // Output to FBO1, GL_COLOR_ATTACHMENT0.
            glBindFramebuffer(GL_FRAMEBUFFER, fbo1Handle);
            glDrawBuffer(GL_COLOR_ATTACHMENT0);
        }
        else {
            // Output to default framebuffer.
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        glViewport(0, 0, winWidth, winHeight); // Viewport for main window.

        glClearColor(0.0, 0.0, 0.0, 1.0);      // Set background color to space black.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

        glm::vec3 ecLightPosition = glm::vec3(viewMat * glm::vec4(lightPosition, 1.0f));
        drawObjectShaderProg.setUniform("LightPosition", ecLightPosition);
        drawObjectShaderProg.setUniform("LightAmbient", lightAmbient);
        drawObjectShaderProg.setUniform("LightDiffuse", lightDiffuse);
        drawObjectShaderProg.setUniform("LightSpecular", lightSpecular);

        RenderObjects(viewMat, projMat);
    }


    // Image processing for bloom effect.
    if (hasBloomEffect)
    {
        imageBloomShaderProg.use(); // Install shader program to rendering pipeline.

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisable(GL_DEPTH_TEST); // No need to use depth testing in image processing.

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_1D, blurFilterTexID);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, originalImageTexID);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, thresholdImageTexID);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, horizBlurImageTexID);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, vertBlurImageTexID);

        imageBloomShaderProg.setUniform("LuminanceThreshold", luminanceThreshold);
        imageBloomShaderProg.setUniform("BlurFilterWidth", blurFilterWidth);

        imageBloomShaderProg.setUniform("BlurFilterTex", 0);
        imageBloomShaderProg.setUniform("OriginalImageTex", 1);
        imageBloomShaderProg.setUniform("ThresholdImageTex", 2);
        imageBloomShaderProg.setUniform("HorizBlurImageTex", 3);
        imageBloomShaderProg.setUniform("VertBlurImageTex", 4);


        // PASS 1.
        {
            imageBloomShaderProg.setUniform("PassNum", 1);

            glBindFramebuffer(GL_FRAMEBUFFER, fbo2Handle);
            glDrawBuffer(GL_COLOR_ATTACHMENT0);
            glViewport(0, 0, fbo2Width, fbo2Height); // Viewport for FBO2. // No need to clear buffer.

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, originalImageTexID);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

            // Draw full-screen quad.
            glBindVertexArray(fullScreenQuadVBO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // PASS 2.
        {
            imageBloomShaderProg.setUniform("PassNum", 2);

            glBindFramebuffer(GL_FRAMEBUFFER, fbo2Handle);
            glDrawBuffer(GL_COLOR_ATTACHMENT1);
            glViewport(0, 0, fbo2Width, fbo2Height); // Viewport for FBO2. // No need to clear buffer.

            // Draw full-screen quad.
            glBindVertexArray(fullScreenQuadVBO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }


        // PASS 3.
        {
            imageBloomShaderProg.setUniform("PassNum", 3);

            glBindFramebuffer(GL_FRAMEBUFFER, fbo2Handle);
            glDrawBuffer(GL_COLOR_ATTACHMENT2);
            glViewport(0, 0, fbo2Width, fbo2Height); // Viewport for FBO2. // No need to clear buffer.

            // Draw full-screen quad.
            glBindVertexArray(fullScreenQuadVBO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // PASS 4.
        {
            imageBloomShaderProg.setUniform("PassNum", 4);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);  // Output to default framebuffer.
            glViewport(0, 0, winWidth, winHeight); // Viewport for main window. // No need to clear buffer.

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, originalImageTexID);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

            // Draw full-screen quad.
            glBindVertexArray(fullScreenQuadVBO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
    }
}



/////////////////////////////////////////////////////////////////////////////
// Draw the objects in the 3D scene.
/////////////////////////////////////////////////////////////////////////////
static void RenderObjects(const glm::mat4 &viewMat, const glm::mat4 &projMat)
{
    glActiveTexture(GL_TEXTURE0);  // All texture maps here use Texture Unit 0.
	drawObjectShaderProg.setUniform("AmbientDiffuseTex", 0);

    {   // Draw the floor (x-z plane).
        glm::mat4 modelMat = glm::mat4(1.0f);
        modelMat = glm::translate(modelMat, glm::vec3(0.0f, -1.0f, -0.0f));
        modelMat = glm::scale(modelMat, glm::vec3(10.0f, 10.0f, 10.0f));

        glm::mat4 modelViewMat = viewMat * modelMat;
        glm::mat4 modelViewProjMat = projMat * modelViewMat;
        glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(modelViewMat)));

        drawObjectShaderProg.setUniform("ModelMatrix", modelMat);
        drawObjectShaderProg.setUniform("ModelViewMatrix", modelViewMat);
        drawObjectShaderProg.setUniform("ModelViewProjMatrix", modelViewProjMat);
        drawObjectShaderProg.setUniform("NormalMatrix", normalMat);

        drawObjectShaderProg.setUniform("MatlAmbient", glm::vec3(0.75f, 0.75f, 1.0f));
        drawObjectShaderProg.setUniform("MatlDiffuse", glm::vec3(0.75f, 0.75f, 1.0f));
        drawObjectShaderProg.setUniform("MatlSpecular", glm::vec3(0.5f, 0.5f, 0.5f));
        drawObjectShaderProg.setUniform("MatlShininess", 16.0f);

        if (objTexID[0] != 0) {
            drawObjectShaderProg.setUniform("UseAmbientDiffuseTex", true);
            glBindTexture(GL_TEXTURE_2D, objTexID[0]);
        }
        else {
            drawObjectShaderProg.setUniform("UseAmbientDiffuseTex", false);
        }

        objModel[0]->render();
    }

    {   // Draw the box.
        glm::mat4 modelMat = glm::mat4(1.0f);
        modelMat = glm::translate(modelMat, glm::vec3(0.0f, -1.0f, -0.0f));
        modelMat = glm::rotate(modelMat, glm::radians(60.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMat = glm::translate(modelMat, glm::vec3(0.8f, 0.01f, -0.8f));
        modelMat = glm::rotate(modelMat, glm::radians(-45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMat = glm::scale(modelMat, glm::vec3(3.0f, 3.5f, 1.5f));
        modelMat = glm::translate(modelMat, glm::vec3(0.0f, 0.5f, 0.0f));

        glm::mat4 modelViewMat = viewMat * modelMat;
        glm::mat4 modelViewProjMat = projMat * modelViewMat;
        glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(modelViewMat)));

        drawObjectShaderProg.setUniform("ModelMatrix", modelMat);
        drawObjectShaderProg.setUniform("ModelViewMatrix", modelViewMat);
        drawObjectShaderProg.setUniform("ModelViewProjMatrix", modelViewProjMat);
        drawObjectShaderProg.setUniform("NormalMatrix", normalMat);

        drawObjectShaderProg.setUniform("MatlAmbient", glm::vec3(1.0f, 0.5f, 0.5f));
        drawObjectShaderProg.setUniform("MatlDiffuse", glm::vec3(1.0f, 0.5f, 0.5f));
        drawObjectShaderProg.setUniform("MatlSpecular", glm::vec3(0.5f, 0.5f, 0.5f));
        drawObjectShaderProg.setUniform("MatlShininess", 32.0f);

        if (objTexID[1] != 0) {
            drawObjectShaderProg.setUniform("UseAmbientDiffuseTex", true);
            glBindTexture(GL_TEXTURE_2D, objTexID[1]);
        }
        else {
            drawObjectShaderProg.setUniform("UseAmbientDiffuseTex", false);
        }

        objModel[1]->render();
    }

    {   // Draw the torus.
        glm::mat4 modelMat = glm::mat4(1.0f);
        modelMat = glm::translate(modelMat, glm::vec3(0.0f, -1.0f, -0.0f));
        modelMat = glm::rotate(modelMat, glm::radians(60.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMat = glm::translate(modelMat, glm::vec3(-2.0f, 1.5f, -1.0f));
        modelMat = glm::scale(modelMat, glm::vec3(1.5f, 1.5f, 1.5f));
        modelMat = glm::rotate(modelMat, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        glm::mat4 modelViewMat = viewMat * modelMat;
        glm::mat4 modelViewProjMat = projMat * modelViewMat;
        glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(modelViewMat)));

        drawObjectShaderProg.setUniform("ModelMatrix", modelMat);
        drawObjectShaderProg.setUniform("ModelViewMatrix", modelViewMat);
        drawObjectShaderProg.setUniform("ModelViewProjMatrix", modelViewProjMat);
        drawObjectShaderProg.setUniform("NormalMatrix", normalMat);

        drawObjectShaderProg.setUniform("MatlAmbient", glm::vec3(0.5f, 1.0f, 0.5f));
        drawObjectShaderProg.setUniform("MatlDiffuse", glm::vec3(0.5f, 1.0f, 0.5f));
        drawObjectShaderProg.setUniform("MatlSpecular", glm::vec3(3.0f, 3.0f, 3.0f));
        drawObjectShaderProg.setUniform("MatlShininess", 32.0f);

        if (objTexID[2] != 0) {
            drawObjectShaderProg.setUniform("UseAmbientDiffuseTex", true);
            glBindTexture(GL_TEXTURE_2D, objTexID[2]);
        }
        else {
            drawObjectShaderProg.setUniform("UseAmbientDiffuseTex", false);
        }

        objModel[2]->render();
    }

    {   // Draw the teapot.
        glm::mat4 modelMat = glm::mat4(1.0f);
        modelMat = glm::translate(modelMat, glm::vec3(0.0f, -1.0f, -0.0f));
        modelMat = glm::rotate(modelMat, glm::radians(60.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMat = glm::translate(modelMat, glm::vec3(-0.5f, 0.0f, 2.0f));
        modelMat = glm::scale(modelMat, glm::vec3(0.8f, 0.8f, 0.8f));
        modelMat = glm::rotate(modelMat, glm::radians(150.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMat = glm::rotate(modelMat, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

        glm::mat4 modelViewMat = viewMat * modelMat;
        glm::mat4 modelViewProjMat = projMat * modelViewMat;
        glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(modelViewMat)));

        drawObjectShaderProg.setUniform("ModelMatrix", modelMat);
        drawObjectShaderProg.setUniform("ModelViewMatrix", modelViewMat);
        drawObjectShaderProg.setUniform("ModelViewProjMatrix", modelViewProjMat);
        drawObjectShaderProg.setUniform("NormalMatrix", normalMat);

        drawObjectShaderProg.setUniform("MatlAmbient", glm::vec3(0.5f, 0.5f, 1.0f));
        drawObjectShaderProg.setUniform("MatlDiffuse", glm::vec3(0.5f, 0.5f, 1.0f));
        drawObjectShaderProg.setUniform("MatlSpecular", glm::vec3(3.0f, 3.0f, 3.0f));
        drawObjectShaderProg.setUniform("MatlShininess", 32.0f);

        if (objTexID[3] != 0) {
            drawObjectShaderProg.setUniform("UseAmbientDiffuseTex", true);
            glBindTexture(GL_TEXTURE_2D, objTexID[3]);
        }
        else {
            drawObjectShaderProg.setUniform("UseAmbientDiffuseTex", false);
        }

        objModel[3]->render();
    }
}



/////////////////////////////////////////////////////////////////////////////
// Set up texture map from image file.
/////////////////////////////////////////////////////////////////////////////
static void SetUpTextureMapFromFile(const char *texMapFile, bool mipmap,
                                    GLuint *texObjID, int *numColorChannels)
{
    // Enable fliping of images vertically when read in.
    // This is to follow OpenGL's image coordinate system, i.e. bottom-leftmost is (0, 0).
    stbi_set_flip_vertically_on_load(true);

    // Read texture image from file.

    int imgWidth, imgHeight, numComponents;

    GLubyte *imgData = stbi_load(texMapFile, &imgWidth, &imgHeight, &numComponents, 0);

    if (imgData == NULL) {
        fprintf(stderr, "Error: Fail to read image file %s.\n", texMapFile);
        exit(EXIT_FAILURE);
    }
    printf("%s (%d x %d, %d components)\n", texMapFile, imgWidth, imgHeight, numComponents);

    GLuint tid;
    glGenTextures(1, &tid);
    glBindTexture(GL_TEXTURE_2D, tid);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (mipmap)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    else
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    const GLint texFormat[4] = { GL_R8, GL_RG8, GL_RGB8, GL_RGBA8 };
    const GLenum dataFormat[4] = { GL_RED, GL_RG, GL_RGB, GL_RGBA };

    if (1 <= numComponents && numComponents <= 4) {
        glTexImage2D(GL_TEXTURE_2D, 0, texFormat[numComponents - 1], imgWidth, imgHeight, 0,
                     dataFormat[numComponents - 1], GL_UNSIGNED_BYTE, imgData);
    }
    else {
        fprintf(stderr, "Error: Unexpected image format.\n");
        exit(EXIT_FAILURE);
    }

    stbi_image_free(imgData);

    if (mipmap) glGenerateMipmap(GL_TEXTURE_2D);

    // Return these values.
    if (texObjID != NULL) *texObjID = tid;
    if (numColorChannels != NULL) *numColorChannels = numComponents;
}


/////////////////////////////////////////////////////////////////////////////
// Set up objects' geometry and texture maps.
/////////////////////////////////////////////////////////////////////////////
static void SetUpObjects()
{
    // Create geometry of 3D object models.
    objModel[0] = new VBOPlane(1.0, 1.0, 32, 32);
    SetUpTextureMapFromFile("images/wood_tile.png", true, &objTexID[0]);

    objModel[1] = new VBOCube;
    SetUpTextureMapFromFile("images/purple_weave.jpg", true, &objTexID[1]);

    objModel[2] = new VBOTorus(0.7, 0.3, 32, 64);
    SetUpTextureMapFromFile("images/wood.png", true, &objTexID[2]);

    objModel[3] = new VBOTeapot(10, glm::mat4(1.0f));
    SetUpTextureMapFromFile("images/green_marble_texture.jpg", true, &objTexID[3]);
}



/////////////////////////////////////////////////////////////////////////////
// Resize texture maps and renderbuffer depth buffer attached to the FBOs.
/////////////////////////////////////////////////////////////////////////////
static void ResizeFBOs()
{
    glBindTexture(GL_TEXTURE_2D, originalImageTexID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, fbo1Width, fbo1Height, 0, GL_RGBA, GL_FLOAT, NULL);

    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderBufID);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, fbo1Width, fbo1Height);

    glBindTexture(GL_TEXTURE_2D, thresholdImageTexID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, fbo2Width, fbo2Height, 0, GL_RGBA, GL_FLOAT, NULL);

    glBindTexture(GL_TEXTURE_2D, horizBlurImageTexID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, fbo2Width, fbo2Height, 0, GL_RGBA, GL_FLOAT, NULL);

    glBindTexture(GL_TEXTURE_2D, vertBlurImageTexID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, fbo2Width, fbo2Height, 0, GL_RGBA, GL_FLOAT, NULL);
}



/////////////////////////////////////////////////////////////////////////////
// Set up all the texture maps to be attached to FBO.
/////////////////////////////////////////////////////////////////////////////
static void SetUpFBOs()
{
    // FBO1
    {
        // Generate and bind FBO1.
        glGenFramebuffers(1, &fbo1Handle);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo1Handle);

        // originalImageTexID
        glGenTextures(1, &originalImageTexID);
        glBindTexture(GL_TEXTURE_2D, originalImageTexID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, fbo1Width, fbo1Height, 0, GL_RGBA, GL_FLOAT, NULL);
        // Bind the texture to the FBO.
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, originalImageTexID, 0);

        // Create the depth buffer.
        glGenRenderbuffers(1, &depthRenderBufID);
        glBindRenderbuffer(GL_RENDERBUFFER, depthRenderBufID);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, fbo1Width, fbo1Height);
        // Bind the depth buffer to the FBO.
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderBufID);
    }

    // FBO2
    {
        // Generate and bind FBO2.
        glGenFramebuffers(1, &fbo2Handle);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo2Handle);

        // thresholdImageTexID
        glGenTextures(1, &thresholdImageTexID);
        glBindTexture(GL_TEXTURE_2D, thresholdImageTexID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, fbo2Width, fbo2Height, 0, GL_RGBA, GL_FLOAT, NULL);
        // Bind the texture to the FBO.
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, thresholdImageTexID, 0);

        // horizBlurImageTexID
        glGenTextures(1, &horizBlurImageTexID);
        glBindTexture(GL_TEXTURE_2D, horizBlurImageTexID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, fbo2Width, fbo2Height, 0, GL_RGBA, GL_FLOAT, NULL);
        // Bind the texture to the FBO.
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, horizBlurImageTexID, 0);

        // vertBlurImageTexID
        glGenTextures(1, &vertBlurImageTexID);
        glBindTexture(GL_TEXTURE_2D, vertBlurImageTexID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, fbo2Width, fbo2Height, 0, GL_RGBA, GL_FLOAT, NULL);
        // Bind the texture to the FBO.
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, vertBlurImageTexID, 0);
    }

    // Revert to default framebuffer for now.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}



/////////////////////////////////////////////////////////////////////////////
// 1D gaussian function.
/////////////////////////////////////////////////////////////////////////////
static float gauss(float x, float sigma2)
{
    const double PI = 3.1415926535897932384626433832795;
    double coeff = 1.0 / sqrt(2.0 * PI * sigma2);
    double expon = -(x * x) / (2.0 * sigma2);
    return (float)(coeff * exp(expon));
}


/////////////////////////////////////////////////////////////////////////////
// Set up 1D texture map that contains the 1D blur filter.
/////////////////////////////////////////////////////////////////////////////
static void SetUpBlurFilterTexture()
{
    float weights[blurFilterWidth];

    int halfWidth = blurFilterWidth / 2;
    float sigma = halfWidth / 2.0;  // Filter spans over 4 sigma (standard deviation).
    float sigma2 = sigma * sigma;

    float sum = 0.0;

    for (int i = 0; i < blurFilterWidth; i++) {
        weights[i] = gauss(i - halfWidth, sigma2);
        sum += weights[i];
    }

    for (int i = 0; i < blurFilterWidth; i++) {
        weights[i] = weights[i] / sum;
    }

    glGenTextures(1, &blurFilterTexID);
    glBindTexture(GL_TEXTURE_1D, blurFilterTexID);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_R32F, blurFilterWidth, 0, GL_RED, GL_FLOAT, weights);
}



/////////////////////////////////////////////////////////////////////////////
// Set up a VBO for the full-screen quad.
/////////////////////////////////////////////////////////////////////////////
static void makeFullScreenQuad()
{
    // Array for full-screen quad.
    // Vertices are already in clip space or in Normalized Device Coordinate (NDC).
    GLfloat verts[] = {
        -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f, -1.0f, 1.0f, 0.0f
    };
    GLfloat texcoords[] = {
        0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
        0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f
    };

    // Set up the vertex buffers.
    unsigned int handle[2];
    glGenBuffers(2, handle);

    glBindBuffer(GL_ARRAY_BUFFER, handle[0]);
    glBufferData(GL_ARRAY_BUFFER, 6 * 3 * sizeof(GLfloat), verts, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, handle[1]);
    glBufferData(GL_ARRAY_BUFFER, 6 * 2 * sizeof(GLfloat), texcoords, GL_STATIC_DRAW);

    // Set up the vertex array object.
    glGenVertexArrays(1, &fullScreenQuadVBO);
    glBindVertexArray(fullScreenQuadVBO);

    glBindBuffer(GL_ARRAY_BUFFER, handle[0]);
    glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, ((GLubyte *)NULL + (0)));
    glEnableVertexAttribArray(0);  // Vertex position

    glBindBuffer(GL_ARRAY_BUFFER, handle[1]);
    glVertexAttribPointer((GLuint)2, 2, GL_FLOAT, GL_FALSE, 0, ((GLubyte *)NULL + (0)));
    glEnableVertexAttribArray(2);  // Texture coordinates

    glBindVertexArray(0);  // Unbind VBO.
}



/////////////////////////////////////////////////////////////////////////////
// The init function. It initializes some OpenGL states.
/////////////////////////////////////////////////////////////////////////////
static void MyInit()
{
    {
        // Set up shader program to do normal rendering.
        try {
            const char vertShaderFile[] = "drawObject.vert";
            const char fragShaderFile[] = "drawObject.frag";
            drawObjectShaderProg.compileShader(vertShaderFile, GLSLShader::VERTEX);
            drawObjectShaderProg.compileShader(fragShaderFile, GLSLShader::FRAGMENT);
            drawObjectShaderProg.link();
            //drawObjectShaderProg.validate();
        }
        catch (GLSLProgramException &e) {
            fprintf(stderr, "Error: %s.\n", e.what());
            exit(EXIT_FAILURE);
        }

        // Set up objects' geometry and texture maps.
        SetUpObjects();
    }

    {
        // Set up shader program to do bloom effect.
        try {
            const char vertShaderFile[] = "imageBloom.vert";
            const char fragShaderFile[] = "imageBloom.frag";
            imageBloomShaderProg.compileShader(vertShaderFile, GLSLShader::VERTEX);
            imageBloomShaderProg.compileShader(fragShaderFile, GLSLShader::FRAGMENT);
            imageBloomShaderProg.link();
            //imageBloomShaderProg.validate();
        }
        catch (GLSLProgramException &e) {
            fprintf(stderr, "Error: %s.\n", e.what());
            exit(EXIT_FAILURE);
        }

        makeFullScreenQuad();
        SetUpBlurFilterTexture();
        SetUpFBOs();
    }

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
// The reshape callback function.
/////////////////////////////////////////////////////////////////////////////
static void MyReshapeFunc(GLFWwindow *window, int w, int h)
{
    winWidth = w;
    winHeight = h;

    fbo1Width = winWidth;
    fbo1Height = winHeight;
    fbo2Width = (int)(fbo2Height * ((double)winWidth / (double)winHeight));

    ResizeFBOs();
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
        else if (key == GLFW_KEY_R) {
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
        else if (key == GLFW_KEY_W) {
            wireframeMode = !wireframeMode;
        }
        else if (key == GLFW_KEY_B) {
            hasBloomEffect = !hasBloomEffect;
        }
    }
}


static void printKeyboardInstruction()
{
    printf("\nKeyboard Instruction:\n");
    printf("* Press ESC or 'Q' to quit.\n");
    printf("* Press 'R' to reset view.\n");
    printf("* Press 'W' to toggle wireframe rendering.\n");
    printf("* Press 'B' to toggle bloom effect.\n\n");
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
    float transScale = 5.0f;

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
    glfwSetMouseButtonCallback(window, MyMouseClickFunc);
    glfwSetCursorPosCallback(window, MyMouseMotionFunc);
    glfwSetKeyCallback(window, MyKeyboardFunc);

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

    printKeyboardInstruction();

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
