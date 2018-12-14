/*
CPE/CSC 471 Lab base code Wood/Dunn/Eckhardt
*/

#include <iostream>
#include <glad/glad.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "GLSL.h"
#include "Program.h"
#include "recordAudio.h"
#include "MatrixStack.h"

#include "WindowManager.h"
#include "camera.h"
#include "Shape.h"

#include "kiss_fft.h"
// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;
using namespace glm;
shared_ptr<Shape> shape;
extern captureAudio actualAudioData;

//#define VR_ENABLED

#ifdef VR_ENABLED
#include "OpenVRclass.h"
OpenVRApplication *vrapp = NULL;
#endif 

#define FFTW_ESTIMATEE (1U << 6)
#define FFT_MAXSIZE 500

/*
Ensures the iGPU does not get used if the system has a dedicated graphics cards
*/
extern "C" {
    _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

bool fft(float *amplitude_on_frequency, int &length)
{


    int N = pow(2, 10);
    BYTE data[MAXS];
    int size = 0;
    actualAudioData.readAudio(data, size);
    length = size / 8;
    if (size == 0)
        return false;

    double *samples = new double[length];
    for (int ii = 0; ii < length; ii++)
    {
        float *f = (float*)&data[ii * 8];
        samples[ii] = (double)(*f);
    }


    kiss_fft_cpx *cx_in = new kiss_fft_cpx[length];
    kiss_fft_cpx *cx_out = new kiss_fft_cpx[length];
    kiss_fft_cfg cfg = kiss_fft_alloc(length, 0, 0, 0);
    for (int i = 0; i < length; ++i)
    {
        cx_in[i].r = samples[i];
        cx_in[i].i = 0;
    }

    kiss_fft(cfg, cx_in, cx_out);

    float amplitude_on_frequency_old[FFT_MAXSIZE];
    for (int i = 0; i < length / 2 && i < FFT_MAXSIZE; ++i)
        amplitude_on_frequency_old[i] = amplitude_on_frequency[i];

    for (int i = 0; i < length / 2 && i < FFT_MAXSIZE; ++i)
        amplitude_on_frequency[i] = sqrt(pow(cx_out[i].i, 2.) + pow(cx_out[i].r, 2.));


    //that looks better, decomment for no filtering: +++++++++++++++++++
    for (int i = 0; i < length / 2 && i < FFT_MAXSIZE; ++i)
    {
        float diff = amplitude_on_frequency_old[i] - amplitude_on_frequency[i];
        float attack_factor = 1;//for going down
        if (amplitude_on_frequency_old[i] < amplitude_on_frequency[i])
            attack_factor = 1; //for going up
        diff *= attack_factor;
        amplitude_on_frequency[i] = amplitude_on_frequency_old[i] - diff;
    }
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


    length /= 2;
    free(cfg);
    return true;
}
#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

BYTE delayfilter(BYTE old, BYTE actual, float mul)
{
    float fold = (float)old;
    float factual = (float)actual;
    float fres = fold - (fold - factual) / mul;
    if (fres > 255) fres = 255;
    else if (fres < 0)fres = 0;
    return (BYTE)fres;
}

double get_last_elapsed_time()
{
    static double lasttime = glfwGetTime();
    double actualtime = glfwGetTime();
    double difference = actualtime - lasttime;
    lasttime = actualtime;
    return difference;
}

class Mouse
{
private:
    bool mousemove = false;

    //Mouse movement vars
    bool firstMouse = true;
    float lastX, lastY, yaw, pitch;

public:
    bool is_mousemove() { return mousemove; }
    void swap_mousemove(GLFWwindow *window)
    {
        if (!mousemove)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            double dcurrentx, dcurrenty;
            glfwGetCursorPos(window, &dcurrentx, &dcurrenty);
            holdx = dcurrentx;
            holdy = dcurrenty;
        }
        else
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        mousemove = !mousemove;
    }

    int holdx, holdy;
    int currentx, currenty;
    //void set_current(bool )
    Mouse() {}
    void process(GLFWwindow *window, vec3 *camerarotation)
    {
        if (!mousemove) return;
        //double dcurrentx, dcurrenty;
        //glfwGetCursorPos(window, &dcurrentx, &dcurrenty);
        //currentx = dcurrentx;
        //currenty = dcurrenty;
        //vec2 diff = vec2(holdx - currentx, holdy - currenty);
        //glfwSetCursorPos(window, (double)holdx, (double)holdy);
        //*camerarotation -= (float)0.005*vec3(diff.y, diff.x, 0);

        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        if (firstMouse)
        {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos;
        lastX = xpos;
        lastY = ypos;

        float sensitivity = 0.05;
        xoffset *= sensitivity;
        yoffset *= sensitivity;

        yaw += xoffset;
        pitch += yoffset;

        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;

        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        *camerarotation = glm::normalize(front);

    }
};


class Application : public EventCallbacks
{

public:

    WindowManager * windowManager = nullptr;
    Mouse mouse;
    camera mycam;
    glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);

    float vizSpeed = 0;
    float fft_buff[10];
    bool showVisualizer = false;

    // Our shader program
    std::shared_ptr<Program> raymarchShader, prog;

    // Contains vertex information for OpenGL
    GLuint VertexArrayID;

    // Data necessary to give our box to OpenGL
    GLuint MeshPosID, MeshTexID, IndexBufferIDBox;

    GLuint VertexArrayIDBox, VertexBufferIDBox, VertexBufferTex;

    // Data necessary to give our triangle to OpenGL
    GLuint VertexBufferID;

    // FFT arrays
    float amplitude_on_frequency[FFT_MAXSIZE];
    float amplitude_on_frequency_10steps[10];

    void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
    {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }

        if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
            showVisualizer = !showVisualizer;
        }

        if (key == GLFW_KEY_W && action == GLFW_PRESS)
        {
            mycam.w = 1;
        }
        if (key == GLFW_KEY_W && action == GLFW_RELEASE)
        {
            mycam.w = 0;
        }
        if (key == GLFW_KEY_S && action == GLFW_PRESS)
        {
            mycam.s = 1;
        }
        if (key == GLFW_KEY_S && action == GLFW_RELEASE)
        {
            mycam.s = 0;
        }
        if (key == GLFW_KEY_A && action == GLFW_PRESS)
        {
            mycam.a = 1;
        }
        if (key == GLFW_KEY_A && action == GLFW_RELEASE)
        {
            mycam.a = 0;
        }
        if (key == GLFW_KEY_D && action == GLFW_PRESS)
        {
            mycam.d = 1;
        }
        if (key == GLFW_KEY_D && action == GLFW_RELEASE)
        {
            mycam.d = 0;
        }
    }

    void mouseCallback(GLFWwindow *window, int button, int action, int mods)
    {
        double posX, posY;

        if (action == GLFW_PRESS)
        {
            mouse.swap_mousemove(windowManager->getHandle());
        }

    }

    //if the window is resized, capture the new size and reset the viewport
    void resizeCallback(GLFWwindow *window, int in_width, int in_height)
    {
        //get the window size - may be different then pixels for retina
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
    }

    /*Note that any gl calls must always happen after a GL state is initialized */
    void initGeom()
    {
        //init rectangle mesh (2 triangles) for the post processing
        glGenVertexArrays(1, &VertexArrayIDBox);
        glBindVertexArray(VertexArrayIDBox);

        //generate vertex buffer to hand off to OGL
        glGenBuffers(1, &VertexBufferIDBox);
        //set the current state to focus on our vertex buffer
        glBindBuffer(GL_ARRAY_BUFFER, VertexBufferIDBox);

        GLfloat *rectangle_vertices = new GLfloat[18];
        // front
        int verccount = 0;

        rectangle_vertices[verccount++] = -1.0, rectangle_vertices[verccount++] = -1.0, rectangle_vertices[verccount++] = -1.0;
        rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = -1.0, rectangle_vertices[verccount++] = -1.0;
        rectangle_vertices[verccount++] = -1.0, rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = -1.0;
        rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = -1.0, rectangle_vertices[verccount++] = -1.0;
        rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = -1.0;
        rectangle_vertices[verccount++] = -1.0, rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = -1.0;


        //actually memcopy the data - only do this once
        glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float), rectangle_vertices, GL_STATIC_DRAW);
        //we need to set up the vertex array
        glEnableVertexAttribArray(0);
        //key function to get up how many elements to pull out at a time (3)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);


        //generate vertex buffer to hand off to OGL
        glGenBuffers(1, &VertexBufferTex);
        //set the current state to focus on our vertex buffer
        glBindBuffer(GL_ARRAY_BUFFER, VertexBufferTex);

        float t = 1. / 100.;
        GLfloat *rectangle_texture_coords = new GLfloat[12];
        int texccount = 0;
        rectangle_texture_coords[texccount++] = 0, rectangle_texture_coords[texccount++] = 0;
        rectangle_texture_coords[texccount++] = 1, rectangle_texture_coords[texccount++] = 0;
        rectangle_texture_coords[texccount++] = 0, rectangle_texture_coords[texccount++] = 1;
        rectangle_texture_coords[texccount++] = 1, rectangle_texture_coords[texccount++] = 0;
        rectangle_texture_coords[texccount++] = 1, rectangle_texture_coords[texccount++] = 1;
        rectangle_texture_coords[texccount++] = 0, rectangle_texture_coords[texccount++] = 1;

        //actually memcopy the data - only do this once
        glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), rectangle_texture_coords, GL_STATIC_DRAW);
        //we need to set up the vertex array
        glEnableVertexAttribArray(2);
        //key function to get up how many elements to pull out at a time (3)
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);


        string resourceDirectory = "../resources";
        // Initialize mesh.
        shape = make_shared<Shape>();
        //shape->loadMesh(resourceDirectory + "/t800.obj");
        shape->loadMesh(resourceDirectory + "/sphere.obj");
        shape->resize();
        shape->init();

        //generate the VAO
        glGenVertexArrays(1, &VertexArrayID);
        glBindVertexArray(VertexArrayID);

        //generate vertex buffer to hand off to OGL
        glGenBuffers(1, &MeshPosID);
        glBindBuffer(GL_ARRAY_BUFFER, MeshPosID);
        vec3 vertices[FFT_MAXSIZE];
        float steps = 10. / (float)FFT_MAXSIZE;

        for (int i = 0; i < FFT_MAXSIZE; i++)
            vertices[i] = vec3(-5. + (float)i*steps, 0.0, 0.0);

        glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * FFT_MAXSIZE, vertices, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glBindVertexArray(0);

    }

    //General OGL initialization - set OGL state here
    void init(const std::string& resourceDirectory)
    {
        GLSL::checkVersion();

        // Set background color.
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        // Enable z-buffer test.
        glEnable(GL_DEPTH_TEST);

        raymarchShader = std::make_shared<Program>();
        raymarchShader->setVerbose(true);
        raymarchShader->setShaderNames(resourceDirectory + "/vert.glsl", resourceDirectory + "/raymarch_frag.glsl");
        if (!raymarchShader->init())
        {
            std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
            exit(1);
        }
        raymarchShader->addUniform("P");
        raymarchShader->addUniform("V");
        raymarchShader->addUniform("M");
        raymarchShader->addUniform("iTime");
        raymarchShader->addUniform("vizSpeed");
        raymarchShader->addUniform("campos");
        raymarchShader->addUniform("cameraFront");
        raymarchShader->addUniform("iResolution");
        raymarchShader->addUniform("VRdir");
        raymarchShader->addUniform("fft_buff");
        raymarchShader->addUniform("VR_Enabled");
        raymarchShader->addAttribute("vertPos");
        raymarchShader->addAttribute("vertTex");

        // Initialize the GLSL program.
        prog = std::make_shared<Program>();
        prog->setVerbose(true);
        prog->setShaderNames(resourceDirectory + "/fft_shader_vertex.glsl", resourceDirectory + "/fft_shader_fragment.glsl");
        if (!prog->init())
        {
            std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
            exit(1);
        }
        prog->addUniform("P");
        prog->addUniform("V");
        prog->addUniform("M");
        prog->addUniform("colorext");
        prog->addAttribute("vertPos");
        prog->addAttribute("vertNor");
        prog->addAttribute("vertTex");

    }

    //*******************
    void aquire_fft_scaling_arrays()
    {
        //get FFT array
        static int length = 0;
        if (fft(amplitude_on_frequency, length))
        {
            //put the height of the frequencies 20Hz to 20000Hz into the height of the line-vertices
            vec3 vertices[FFT_MAXSIZE];
            glBindBuffer(GL_ARRAY_BUFFER, MeshPosID);
            float steps = 10. / (float)FFT_MAXSIZE;
            for (int i = 0; i < FFT_MAXSIZE; i++)
            {
                float step = i / (float)length;
                step *= 20;

                float height = 0;
                if (i < length)
                    height = amplitude_on_frequency[i] * 0.05 * (1 + step) * 4;
                vertices[i] = vec3(-5. + (float)i*steps, height, 0.0);
            }

            //int interval = 20;
            //float averageHeight;
            //for (int i = 0; i < FFT_MAXSIZE; i += interval)
            //{
            //    averageHeight = 0;
            //    for (int j = 0; j < interval; j++) {
            //        float step = i / (float)length;
            //        step *= 20;
            //        averageHeight += amplitude_on_frequency[i] * 0.05 * (1 + step) * 3;
            //    }
            //    averageHeight /= interval;
            //    for (int k = i; k < i + interval; k++) {
            //        vertices[k] = vec3(-5. + (float)k*steps, averageHeight, 0.0);
            //    }
            //}


            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec3) * FFT_MAXSIZE, vertices);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            //calculate the average amplitudes for the 10 spheres
            for (int i = 0; i < 10; i++)
                amplitude_on_frequency_10steps[i] = 0;

            int mean_range = length / 10;
            int bar = 0;
            int count = 0;

            for (int i = 0; ; i++, count++)
            {
                if (mean_range == count)
                {
                    count = -1;
                    amplitude_on_frequency_10steps[bar] /= (float)mean_range;
                    bar++;
                    if (bar == 10)break;
                }
                if (i < length && i < FFT_MAXSIZE)
                    amplitude_on_frequency_10steps[bar] += amplitude_on_frequency[i];
            }
        }
    }
    void render()
    {
        static double count = 0;
        double frametime = get_last_elapsed_time();
        count += frametime;

        // Get current frame buffer size.
        int width, height;
        glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
        float aspect = width / (float)height;
        glViewport(0, 0, width, height);

        // Clear framebuffer.
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 V, M, P; //View, Model and Perspective matrix

        V = glm::mat4(1);
        P = glm::perspective((float)(3.14159 / 4.), (float)((float)width / (float)height), 0.01f, 100000.0f); //so much type casting... GLM metods are quite funny ones


        // Draw the box using GLSL.
        prog->bind();

        //Set the FFT arrays
        aquire_fft_scaling_arrays();


        //send the matrices to the shaders

        glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, &P[0][0]);
        glUniformMatrix4fv(prog->getUniform("V"), 1, GL_FALSE, &V[0][0]);

        for (int i = 0; i < 10; i++)
        {
            vec3 color = vec3(1, 0, (float)i / 10.);
            glUniform3fv(prog->getUniform("colorext"), 1, &color.x);
            float scaling = amplitude_on_frequency_10steps[i] * 0.2;
            M = glm::translate(glm::mat4(1.0f), vec3(-4.5 + i, 2, -9)) * glm::scale(mat4(1), vec3(0.2 + scaling, 0.2 + scaling, 0.2 + scaling));
            glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);
            shape->draw(prog, FALSE);
        }
        vec3 color = vec3(0, 1, 0);
        glUniform3fv(prog->getUniform("colorext"), 1, &color.x);
        M = glm::translate(glm::mat4(1.0f), vec3(0, -2, -9));
        glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);

        glBindVertexArray(VertexArrayID);
        glDrawArrays(GL_LINE_STRIP, 0, FFT_MAXSIZE - 1);
        prog->unbind();
    }

    void updateFFTValues() {
        aquire_fft_scaling_arrays();

        for (int i = 0; i < 10; i++) {
            fft_buff[i] = amplitude_on_frequency_10steps[i] * 2;
            //fft_buff[i] = amplitude_on_frequency_10steps[i] * 0.1;
            //cout << fft_buff[i] << " ";
        }

        vizSpeed += (fft_buff[0] + fft_buff[1] + fft_buff[2]);
    }

    void render_fractals()
    {

        int width, height;
        glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
        float aspect = width / (float)height;
        glViewport(0, 0, width, height);
        float iResolution[2] = { width, height };

        // Clear framebuffer.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //for (int i = 0; i < 10; i++)
        //{
        //    vec3 color = vec3(1, 0, (float)i / 10.);
        //    glUniform3fv(prog->getUniform("colorext"), 1, &color.x);
        //    float scaling = amplitude_on_frequency_10steps[i] * 0.2;
        //    M = glm::translate(glm::mat4(1.0f), vec3(-4.5 + i, 2, -9)) * glm::scale(mat4(1), vec3(0.2 + scaling, 0.2 + scaling, 0.2 + scaling));
        //    glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);
        //    shape->draw(prog, FALSE);
        //}

        updateFFTValues();

        raymarchShader->bind();

        glUniform1i(raymarchShader->getUniform("VR_Enabled"), 0);
        glUniform1fv(raymarchShader->getUniform("fft_buff"), 10, fft_buff);
        glUniform3fv(raymarchShader->getUniform("campos"), 1, &mycam.pos.x);
        glUniform3fv(raymarchShader->getUniform("cameraFront"), 1, &cameraFront.x);
        glUniform1f(raymarchShader->getUniform("iTime"), glfwGetTime());
        glUniform1f(raymarchShader->getUniform("vizSpeed"), vizSpeed);
        glUniform2fv(raymarchShader->getUniform("iResolution"), 1, iResolution);
        glBindVertexArray(VertexArrayIDBox);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        raymarchShader->unbind();

        //update mouse
        mouse.process(windowManager->getHandle(), &cameraFront);
        //update camera
        mycam.process(&cameraFront);

        //cout << mycam.pos.x << "," << mycam.pos.y << "," << mycam.pos.z << endl;
    }

    void render_vr(int width, int height, glm::mat4 VRheadmatrix) {
        //int width, height;
        glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
        float aspect = width / (float)height;
        // glViewport(0, 0, width, height);
        float iResolution[2] = { width, height };

        //mycam.trackingM = VRheadmatrix;

        // Clear framebuffer.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        updateFFTValues();

        raymarchShader->bind();

        //cout << glm::to_string(VRheadmatrix) << endl;

        //vec2 VR_Resolution = vec2(vrapp->get_render_width(), vrapp->get_render_height());
        //vec2 VR_Resolution = vec2(1080, 1200);
        vec2 VR_Resolution = vec2(width, height);
        //cout << VR_Resolution.x << ", " << VR_Resolution.y << endl;

        glUniform1i(raymarchShader->getUniform("VR_Enabled"), 1);
        glUniform1fv(raymarchShader->getUniform("fft_buff"), 10, fft_buff);
        glUniform3fv(raymarchShader->getUniform("campos"), 1, &mycam.pos.x);
        glUniform3fv(raymarchShader->getUniform("cameraFront"), 1, &cameraFront.x);
        glUniform1f(raymarchShader->getUniform("iTime"), glfwGetTime());
        glUniform1f(raymarchShader->getUniform("vizSpeed"), vizSpeed);
        glUniform2fv(raymarchShader->getUniform("iResolution"), 1, &VR_Resolution.x);
        glUniformMatrix4fv(raymarchShader->getUniform("VRdir"), 1, GL_FALSE, &VRheadmatrix[0][0]);
        glBindVertexArray(VertexArrayIDBox);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        raymarchShader->unbind();

        //update mouse
        mouse.process(windowManager->getHandle(), &cameraFront);
        //update camera
        mycam.process(&cameraFront);

    }

};

Application *application = NULL;
void renderfct(int w, int h, glm::mat4 VRheadmatrix)
{
    application->render_vr(w, h, VRheadmatrix);
}

//******************************************************************************************
int main(int argc, char **argv)
{
    std::string resourceDir = "../resources"; // Where the resources are loaded from
    if (argc >= 2)
    {
        resourceDir = argv[1];
    }

    application = new Application();

    /* your main will always include a similar set up to establish your window
        and GL context, etc. */
    WindowManager * windowManager = new WindowManager();

#ifdef VR_ENABLED
    vrapp = new OpenVRApplication();
    windowManager->init(vrapp->get_render_width(), vrapp->get_render_height());
    vrapp->init_buffers(resourceDir);
#else
    windowManager->init(1280, 720);
#endif 

    windowManager->setEventCallbacks(application);
    application->windowManager = windowManager;

    /* This is the code that will likely change program to program as you
        may need to initialize or set up different data and state */
        // Initialize scene.
    application->init(resourceDir);
    application->initGeom();


    thread t1(start_recording);
    // Loop until the user closes the window.
    while (!glfwWindowShouldClose(windowManager->getHandle()))
    {

#ifdef VR_ENABLED
        vrapp->render_to_VR(renderfct);
        vrapp->render_to_screen(1);
#else

        //Render the music visualizer.
        if (application->showVisualizer)
            application->render();
        //Render raymarched scene
        else
            application->render_fractals();
#endif 

        // Swap front and back buffers.
        glfwSwapBuffers(windowManager->getHandle());
        // Poll for and process events.
        glfwPollEvents();
    }

    t1.join();

    // Quit program.
    windowManager->shutdown();
    return 0;
}
