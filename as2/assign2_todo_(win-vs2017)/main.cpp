//============================================================================
// GROUP NUMBER: 1
//
// STUDENT NAME: Duan Haoyi
// NUS User ID.: t0925750
//
// STUDENT NAME: Zhu Yuzhe
// NUS User ID.: t0925953
//
// STUDENT NAME: Sun Hanxi 
// NUS User ID.: t0925933
//
// COMMENTS TO GRADER: 
//
//============================================================================

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
#include "helper/vboplane.h"
#include "helper/vbocube.h"
#include "helper/vbotorus.h"


// Shaders' filenames.
const char vertShaderFile[] = "shader.vert";
const char fragShaderFile[] = "shader.frag";

GLSLProgram shaderProg;  // Contains the shader program object.


// Light info.
const GLfloat initial_lightPosition[] = { 5.0f, 3.0f, 10.0f, 0.0f };  // Directional light. Assumed in eye space.
const GLfloat lightAmbient[] = { 0.1f, 0.1f, 0.1f, 1.0f };
const GLfloat lightDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat lightSpecular[] = { 0.8f, 0.8f, 0.8f, 1.0f };
GLfloat lightPosition[4];  // Directional light. Assumed in eye space.

const GLfloat lightPosIncr = 0.5f; // Step size to move light position in x & y directions.


const float skyboxSize = 50.0f;  // Desired width of the skybox.

// The skybox cube model.
Drawable *skyboxModel;

// 3D object model (brick cube).
Drawable *obj3DModel1;

// 3D object model (wooden cube).
Drawable *obj3DModel2;


// For window and viewport size.
int winWidth = 800;     // Window width in pixels.
int winHeight = 600;    // Window height in pixels.

bool wireframeMode = false;

// For trackball.
double prevMouseX, prevMouseY;
bool mouseLeftPressed;
bool mouseMiddlePressed;
bool mouseRightPressed;
float cam_curr_quat[4];
float cam_prev_quat[4];
float cam_eye[3], cam_lookat[3], cam_up[3];
const float initial_cam_pos[3] = { 0.0, 0.0, 3.0f };  // World coordinates.




/////////////////////////////////////////////////////////////////////////////
// Set up the environment cubemap.
/////////////////////////////////////////////////////////////////////////////
static void SetupEnvMap(void) {

    const int numImages = 6;
    const GLenum texUnit = GL_TEXTURE0;

    // Cubemap images' filenames.
    const char *cubemapFile[numImages] = {
        "images/cm2_right.png", "images/cm2_left.png",
        "images/cm2_top.png", "images/cm2_bottom.png",
        "images/cm2_back.png", "images/cm2_front.png"
    };


    GLuint target[numImages] = {
        GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
    };


    glActiveTexture(texUnit);
    GLuint tid;
    glGenTextures(1, &tid);
    glBindTexture(GL_TEXTURE_CUBE_MAP, tid);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    // Enable fliping of images vertically when read in.
    // This is to follow OpenGL's image coordinate system, i.e. bottom-leftmost is (0, 0).
    stbi_set_flip_vertically_on_load(true);

    // Read texture images from files.
    for (int t = 0; t < numImages; t++) {

        int imgWidth, imgHeight, numComponents;

        GLubyte *imgData = stbi_load(cubemapFile[t], &imgWidth, &imgHeight, &numComponents, 0);

        if (imgData == NULL) {
            fprintf(stderr, "Error: Fail to read image file %s.\n", cubemapFile[t]);
            exit(EXIT_FAILURE);
        }
        printf("%s (%d x %d, %d components)\n", cubemapFile[t], imgWidth, imgHeight, numComponents);

        if (numComponents == 1) {
            glTexImage2D(target[t], 0, GL_R8, imgWidth, imgHeight, 0,
                GL_RED, GL_UNSIGNED_BYTE, imgData);
        }
        else if (numComponents == 3) {
            glTexImage2D(target[t], 0, GL_RGB8, imgWidth, imgHeight, 0,
                GL_RGB, GL_UNSIGNED_BYTE, imgData);
        }
        else {
            fprintf(stderr, "Error: Unexpected image format.\n");
            exit(EXIT_FAILURE);
        }

        stbi_image_free(imgData);
    }

    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
}



/////////////////////////////////////////////////////////////////////////////
// Set up all the texture maps to be used.
/////////////////////////////////////////////////////////////////////////////
static void SetupTextureMaps(void) {

    const int numTexMaps = 3;

    const char *texMapFile[numTexMaps] = {
        "images/brick_diffuse_map.png",
        "images/brick_normal_map.png",
        "images/wood_diffuse_map.png"
    };

    const GLenum texUnit[numTexMaps] = { GL_TEXTURE1, GL_TEXTURE2, GL_TEXTURE3 };

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
// The init function. It initializes some OpenGL states.
/////////////////////////////////////////////////////////////////////////////
static void MyInit(void)
{
    // Install shaders.
    try {
        shaderProg.compileShader(vertShaderFile, GLSLShader::VERTEX);
        shaderProg.compileShader(fragShaderFile, GLSLShader::FRAGMENT);
        shaderProg.link();
        // shaderProg.validate();
        shaderProg.use(); // Install shader program to rendering pipeline.
    }
    catch(GLSLProgramException &e) {
        fprintf(stderr, "Error: %s.\n", e.what());
        exit(EXIT_FAILURE);
    }

    // Create geometry of 3D object models.
    skyboxModel = new VBOCube;
    obj3DModel1 = new VBOCube;
    obj3DModel2 = new VBOCube;

    // Set up environment cubemap.
    SetupEnvMap();

    // Set up texture maps.
    SetupTextureMaps();

    // Initialize some OpenGL states.
    glClearColor(0.0, 0.0, 0.0, 1.0); // Set background color to space black.
    glEnable(GL_DEPTH_TEST); // Use depth-buffer for hidden surface removal.

    // Initialize light position.
    lightPosition[0] = initial_lightPosition[0];
    lightPosition[1] = initial_lightPosition[1];
    lightPosition[2] = initial_lightPosition[2];
    lightPosition[3] = initial_lightPosition[3];

    // Initialization for trackball.
    trackball(cam_curr_quat, 0, 0, 0, 0);
    cam_eye[0] = initial_cam_pos[0];
    cam_eye[1] = initial_cam_pos[1];
    cam_eye[2] = initial_cam_pos[2];
    cam_lookat[0] = 0.0f;
    cam_lookat[1] = 0.0f;
    cam_lookat[2] = 0.0f;
    cam_up[0] = 0.0f;
    cam_up[1] = 1.0f;
    cam_up[2] = 0.0f;
}



/////////////////////////////////////////////////////////////////////////////
// The draw function.
/////////////////////////////////////////////////////////////////////////////
static void MyDrawFunc(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (wireframeMode)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    //=======================================================================
    // Pass texture unit numbers of texture maps to shaders.
    //=======================================================================

    shaderProg.setUniform("EnvMap", 0);
    shaderProg.setUniform("BrickDiffuseMap", 1);
    shaderProg.setUniform("BrickNormalMap", 2);
    shaderProg.setUniform("WoodDiffuseMap", 3);

    //=======================================================================
    // Set up View and Projection matrices.
    //=======================================================================

    // Perspective projection matrix.
    glm::mat4 projMat = glm::perspective(glm::radians(60.0f), (float)winWidth / winHeight, 0.1f, 100.0f);

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

    //=======================================================================
    // Set up transformations and draw the skybox.
    //=======================================================================
    {
        // Actual final camera position in World coordinates.
        glm::vec3 camWorldPos = glm::transpose(glm::mat3(camRotMat)) * glm::vec3(cam_eye[0], cam_eye[1], cam_eye[2]);


        /////////////////////////////////////////////////////////////////////////////
        // TASK 1:
        // Set up the Modelling Matrix modelMat0 for scaling the skybox cube to the
        // size skyboxSize x skyboxSize x skyboxSize, and for positioning it at the
        // correct position. Note that the skybox cube is initially a 1x1x1 cube
        // centered at the origin.
        /////////////////////////////////////////////////////////////////////////////

        glm::mat4 modelMat0 = glm::mat4(1.0f);

        ///////////////////////////////////
        // TASK 1: WRITE YOUR CODE HERE. //
        ///////////////////////////////////
		modelMat0 = glm::translate(modelMat0, glm::transpose(glm::mat3(camRotMat)) * glm::vec3(cam_eye[0], cam_eye[1], cam_eye[2]));
		modelMat0 = glm::scale(modelMat0, glm::vec3(skyboxSize, skyboxSize, skyboxSize));

        glm::mat4 modelViewMat0 = viewMat * modelMat0;
        glm::mat4 modelViewProjMat0 = projMat * modelViewMat0;
        glm::mat3 normalMat0 = glm::transpose(glm::inverse(glm::mat3(modelViewMat0)));

        shaderProg.setUniform("ViewMatrix", viewMat);
        shaderProg.setUniform("ModelViewMatrix", modelViewMat0);
        shaderProg.setUniform("ModelViewProjMatrix", modelViewProjMat0);
        shaderProg.setUniform("NormalMatrix", normalMat0);
        shaderProg.setUniform("WhichObj", 0);  // 0 -- draw skybox, 1 -- draw brick cube, 2 -- draw wooden cube.

        // Draw skybox.
        skyboxModel->render();
    }

    //=======================================================================
    // Set up transformations and other uniform variables and draw the brick cube.
    //=======================================================================
    {
        // Translate it to the left side.
        glm::mat4 modelMat1 = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 0.0f, 0.0f));

        // Fit to -1, 1
        modelMat1 = glm::scale(modelMat1, glm::vec3(1.5f, 1.5f, 1.5f));

        // Rotate 0 degrees about x-axis.
        modelMat1 = glm::rotate(modelMat1, glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));

        glm::mat4 modelViewMat1 = viewMat * modelMat1;
        glm::mat4 modelViewProjMat1 = projMat * modelViewMat1;
        glm::mat3 normalMat1 = glm::transpose(glm::inverse(glm::mat3(modelViewMat1)));

        shaderProg.setUniform("ViewMatrix", viewMat);
        shaderProg.setUniform("ModelViewMatrix", modelViewMat1);
        shaderProg.setUniform("ModelViewProjMatrix", modelViewProjMat1);
        shaderProg.setUniform("NormalMatrix", normalMat1);

        shaderProg.setUniform("LightPosition",
            glm::vec4(lightPosition[0], lightPosition[1], lightPosition[2], lightPosition[3]));
        shaderProg.setUniform("LightAmbient",
            glm::vec4(lightAmbient[0], lightAmbient[1], lightAmbient[2], lightAmbient[3]));
        shaderProg.setUniform("LightDiffuse",
            glm::vec4(lightDiffuse[0], lightDiffuse[1], lightDiffuse[2], lightDiffuse[3]));
        shaderProg.setUniform("LightSpecular",
            glm::vec4(lightSpecular[0], lightSpecular[1], lightSpecular[2], lightSpecular[3]));

        shaderProg.setUniform("WhichObj", 1);  // 0 -- draw skybox, 1 -- draw brick cube, 2 -- draw wooden cube.

        // Draw brick cube.
        obj3DModel1->render();
    }

    //=======================================================================
    // Set up transformations and other uniform variables and draw the wooden cube.
    //=======================================================================
    {
        // Translate it to the right side.
        glm::mat4 modelMat2 = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f));

        // Make it slightly larger.
        modelMat2 = glm::scale(modelMat2, glm::vec3(1.5f, 1.5f, 1.5f));

        // Rotate 0 degrees about x-axis.
        modelMat2 = glm::rotate(modelMat2, glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));

        glm::mat4 modelViewMat2 = viewMat * modelMat2;
        glm::mat4 modelViewProjMat2 = projMat * modelViewMat2;
        glm::mat3 normalMat2 = glm::transpose(glm::inverse(glm::mat3(modelViewMat2)));

        shaderProg.setUniform("ViewMatrix", viewMat);
        shaderProg.setUniform("ModelViewMatrix", modelViewMat2);
        shaderProg.setUniform("ModelViewProjMatrix", modelViewProjMat2);
        shaderProg.setUniform("NormalMatrix", normalMat2);

        shaderProg.setUniform("LightPosition",
            glm::vec4(lightPosition[0], lightPosition[1], lightPosition[2], lightPosition[3]));
        shaderProg.setUniform("LightAmbient",
            glm::vec4(lightAmbient[0], lightAmbient[1], lightAmbient[2], lightAmbient[3]));
        shaderProg.setUniform("LightDiffuse",
            glm::vec4(lightDiffuse[0], lightDiffuse[1], lightDiffuse[2], lightDiffuse[3]));
        shaderProg.setUniform("LightSpecular",
            glm::vec4(lightSpecular[0], lightSpecular[1], lightSpecular[2], lightSpecular[3]));

        shaderProg.setUniform("WhichObj", 2);  // 0 -- draw skybox, 1 -- draw brick cube, 2 -- draw wooden cube.

        // Draw wooden cube.
        obj3DModel2->render();
    }
}



/////////////////////////////////////////////////////////////////////////////
// The reshape callback function.
/////////////////////////////////////////////////////////////////////////////
static void MyReshapeFunc(GLFWwindow *window, int w, int h)
{
    winWidth = w;
    winHeight = h;
    glViewport(0, 0, w, h);
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
            lightPosition[0] = initial_lightPosition[0];
            lightPosition[1] = initial_lightPosition[1];
            lightPosition[2] = initial_lightPosition[2];
            lightPosition[3] = initial_lightPosition[3];

            // Reset the trackball.
            trackball(cam_curr_quat, 0, 0, 0, 0);
            cam_eye[0] = initial_cam_pos[0];
            cam_eye[1] = initial_cam_pos[1];
            cam_eye[2] = initial_cam_pos[2];
            cam_lookat[0] = 0.0f;
            cam_lookat[1] = 0.0f;
            cam_lookat[2] = 0.0f;
            cam_up[0] = 0.0f;
            cam_up[1] = 1.0f;
            cam_up[2] = 0.0f;
        }

        // Move the light.
        else if (key == GLFW_KEY_UP) {
            lightPosition[1] += lightPosIncr;
        }
        else if (key == GLFW_KEY_DOWN) {
            lightPosition[1] -= lightPosIncr;
        }
        else if (key == GLFW_KEY_LEFT) {
            lightPosition[0] -= lightPosIncr;
        }
        else if (key == GLFW_KEY_RIGHT) {
            lightPosition[0] += lightPosIncr;
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
