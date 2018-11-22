/*
CPE/CSC 471 Lab base code Wood/Dunn/Eckhardt
*/

#include <iostream>
#include <glad/glad.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "GLSL.h"
#include "Program.h"
#include "MatrixStack.h"

#include "WindowManager.h"
#include "Shape.h"
// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace std;
using namespace glm;
shared_ptr<Shape> shape;

/*
Ensures the iGPU does not get used if the system has a dedicated graphics cards
*/
extern "C" {
    _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

double get_last_elapsed_time()
{
	static double lasttime = glfwGetTime();
	double actualtime =glfwGetTime();
	double difference = actualtime- lasttime;
	lasttime = actualtime;
	return difference;
}
class camera
{
public:
	glm::vec3 pos, rot;
	int w, a, s, d;
	camera()
	{
		w = a = s = d = 0;
		pos = rot = glm::vec3(0, 0, 0);
	}
	glm::mat4 process(double ftime)
	{
		float speed = 0;
		if (w == 1)
		{
			speed = 10*ftime;
		}
		else if (s == 1)
		{
			speed = -10*ftime;
		}
		float yangle=0;
		if (a == 1)
			yangle = -3*ftime;
		else if(d==1)
			yangle = 3*ftime;
		rot.y += yangle;
		glm::mat4 R = glm::rotate(glm::mat4(1), rot.y, glm::vec3(0, 1, 0));
		glm::vec4 dir = glm::vec4(0, 0, speed,1);
		dir = dir*R;
		pos += glm::vec3(dir.x, dir.y, dir.z);
		glm::mat4 T = glm::translate(glm::mat4(1), pos);
		return R*T;
	}
};

camera mycam;

class Application : public EventCallbacks
{

public:

	WindowManager * windowManager = nullptr;

	// Our shader program
	std::shared_ptr<Program> raymarchShader;

	// Contains vertex information for OpenGL
	GLuint VertexArrayID;

	// Data necessary to give our box to OpenGL
	GLuint MeshPosID, MeshTexID, IndexBufferIDBox;

    GLuint VertexArrayIDBox, VertexBufferIDBox, VertexBufferTex;

    // Data necessary to give our triangle to OpenGL
    GLuint VertexBufferID;

	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
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

	// callback for the mouse when clicked move the triangle when helper functions
	// written
	void mouseCallback(GLFWwindow *window, int button, int action, int mods)
	{
		double posX, posY;
		float newPt[2];
		if (action == GLFW_PRESS)
		{
			glfwGetCursorPos(window, &posX, &posY);
			std::cout << "Pos X " << posX <<  " Pos Y " << posY << std::endl;

			//change this to be the points converted to WORLD
			//THIS IS BROKEN< YOU GET TO FIX IT - yay!
			newPt[0] = 0;
			newPt[1] = 0;

			std::cout << "converted:" << newPt[0] << " " << newPt[1] << std::endl;
			glBindBuffer(GL_ARRAY_BUFFER, MeshPosID);
			//update the vertex array with the updated points
			glBufferSubData(GL_ARRAY_BUFFER, sizeof(float)*6, sizeof(float)*2, newPt);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
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
#define MESHSIZE 100
	void init_mesh()
	{
		//generate the VAO
		glGenVertexArrays(1, &VertexArrayID);
		glBindVertexArray(VertexArrayID);

		//generate vertex buffer to hand off to OGL
		glGenBuffers(1, &MeshPosID);
		glBindBuffer(GL_ARRAY_BUFFER, MeshPosID);
		vec3 vertices[MESHSIZE * MESHSIZE * 4];
		for(int x=0;x<MESHSIZE;x++)
			for (int z = 0; z < MESHSIZE; z++)
				{
				vertices[x * 4 + z*MESHSIZE * 4 + 0] = vec3(0.0, 0.0, 0.0) + vec3(x, 0, z);
				vertices[x * 4 + z*MESHSIZE * 4 + 1] = vec3(1.0, 0.0, 0.0) + vec3(x, 0, z);
				vertices[x * 4 + z*MESHSIZE * 4 + 2] = vec3(1.0, 0.0, 1.0) + vec3(x, 0, z);
				vertices[x * 4 + z*MESHSIZE * 4 + 3] = vec3(0.0, 0.0, 1.0) + vec3(x, 0, z);
				}
		glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * MESHSIZE * MESHSIZE * 4, vertices, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		//tex coords
		float t = 1. / 100;
		vec2 tex[MESHSIZE * MESHSIZE * 4];
		for (int x = 0; x<MESHSIZE; x++)
			for (int y = 0; y < MESHSIZE; y++)
			{
				tex[x * 4 + y*MESHSIZE * 4 + 0] = vec2(0.0, 0.0)+ vec2(x, y)*t;
				tex[x * 4 + y*MESHSIZE * 4 + 1] = vec2(t, 0.0)+ vec2(x, y)*t;
				tex[x * 4 + y*MESHSIZE * 4 + 2] = vec2(t, t)+ vec2(x, y)*t;
				tex[x * 4 + y*MESHSIZE * 4 + 3] = vec2(0.0, t)+ vec2(x, y)*t;
			}
		glGenBuffers(1, &MeshTexID);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, MeshTexID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * MESHSIZE * MESHSIZE * 4, tex, GL_STATIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

		glGenBuffers(1, &IndexBufferIDBox);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferIDBox);
		GLushort elements[MESHSIZE * MESHSIZE * 6];
		int ind = 0;
		for (int i = 0; i<MESHSIZE * MESHSIZE * 6; i+=6, ind+=4)
			{
			elements[i + 0] = ind + 0;
			elements[i + 1] = ind + 1;
			elements[i + 2] = ind + 2;
			elements[i + 3] = ind + 0;
			elements[i + 4] = ind + 2;
			elements[i + 5] = ind + 3;
			}			
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort)*MESHSIZE * MESHSIZE * 6, elements, GL_STATIC_DRAW);
		glBindVertexArray(0);
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

        rectangle_vertices[verccount++] = 0.0, rectangle_vertices[verccount++] = 0.0, rectangle_vertices[verccount++] = 0.0;
        rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 0.0, rectangle_vertices[verccount++] = 0.0;
        rectangle_vertices[verccount++] = 0.0, rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 0.0;
        rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 0.0, rectangle_vertices[verccount++] = 0.0;
        rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 0.0;
        rectangle_vertices[verccount++] = 0.0, rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 0.0;


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

        //~~~~~~~~~~~~~~~~~~~~~~

		//glEnable(GL_BLEND);
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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
        raymarchShader->addUniform("iResolution");
        raymarchShader->addAttribute("vertPos");
        raymarchShader->addAttribute("vertTex");
	}

    void render_to_screen()
    {

        int width, height;
        glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
        float aspect = width / (float)height;
        glViewport(0, 0, width, height);

        auto P = std::make_shared<MatrixStack>();
        P->pushMatrix();
        P->perspective(70., width, height, 0.1, 100.0f);
        glm::mat4 M, V, S, T;

        V = glm::mat4(1);

        // Clear framebuffer.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        raymarchShader->bind();

        float iResolution[2] = { width, height };

        M = glm::scale(glm::mat4(1), glm::vec3(1.2, 1, 1)) * glm::translate(glm::mat4(1), glm::vec3(-0.5, -0.5, -1));
        glUniformMatrix4fv(raymarchShader->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
        glUniformMatrix4fv(raymarchShader->getUniform("V"), 1, GL_FALSE, &V[0][0]);
        glUniformMatrix4fv(raymarchShader->getUniform("M"), 1, GL_FALSE, &M[0][0]);
        glUniform1f(raymarchShader->getUniform("iTime"), glfwGetTime());
        glUniform2fv(raymarchShader->getUniform("iResolution"), 1, iResolution);
        glBindVertexArray(VertexArrayIDBox);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        raymarchShader->unbind();

    }


	

};
//******************************************************************************************
int main(int argc, char **argv)
{
	std::string resourceDir = "../resources"; // Where the resources are loaded from
	if (argc >= 2)
	{
		resourceDir = argv[1];
	}

	Application *application = new Application();

	/* your main will always include a similar set up to establish your window
		and GL context, etc. */
	WindowManager * windowManager = new WindowManager();
	windowManager->init(720, 720);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	/* This is the code that will likely change program to program as you
		may need to initialize or set up different data and state */
	// Initialize scene.
	application->init(resourceDir);
	application->initGeom();

	// Loop until the user closes the window.
	while(! glfwWindowShouldClose(windowManager->getHandle()))
	{
		// Render scene.
		//application->render();
        application->render_to_screen();

		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}

	// Quit program.
	windowManager->shutdown();
	return 0;
}
