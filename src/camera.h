#pragma once


#ifndef LAB471_CAMERA_H_INCLUDED
#define LAB471_CAMERA_H_INCLUDED

#include <stack>
#include <memory>

#include "glm/glm.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"

#define PI 3.1415926
#define PIH 3.1415926/2.
#define MOVESPEED 1

class camera
{
private:
	glm::mat4 View;
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
public:
	glm::vec3 pos;
	glm::vec3 rot;
	int w, a, s, d;
	
	glm::mat4 get_viewmatrix() { return View; }
	camera()
	{
		w = a = s = d = 0;
		pos = rot = glm::vec3(0, 0, 0);
		pos = glm::vec3(0, 0, 0);
		rot = glm::vec3(0, 0, 0);
	}
	void process(glm::vec3 *cameraFront)
	{
		float going_forward = 0.0;
		if (w == 1)
			//going_forward -= MOVESPEED;
            pos += *cameraFront * (float)MOVESPEED;
		if (s == 1)
			//going_forward += MOVESPEED;
            pos -= *cameraFront * (float)MOVESPEED;
		if (a == 1)
			//rot.y -= TURNSPEED;
            pos -= glm::normalize(glm::cross(*cameraFront, cameraUp)) * (float)MOVESPEED;
		if (d == 1)
			//rot.y += TURNSPEED;
            pos += glm::normalize(glm::cross(*cameraFront, cameraUp)) * (float)MOVESPEED;

		if (rot.x > PIH) rot.x = PIH;
		if (rot.x < -PIH) rot.x = -PIH;
		glm::mat4 Ry = glm::rotate(glm::mat4(1), rot.y, glm::vec3(0, 1, 0));
		glm::mat4 Rx = glm::rotate(glm::mat4(1), rot.x, glm::vec3(1, 0, 0));

		glm::mat4 R = Rx*Ry;
		
		glm::vec4 rpos = glm::vec4(0, 0, -going_forward, 1);

		rpos = rpos *R;
		pos.x -= rpos.x;
		pos.y -= rpos.y;
		pos.z -= rpos.z;

		glm::mat4 T = glm::translate(glm::mat4(1), glm::vec3(pos.x, pos.y, pos.z));
		View = R*T;
	}

};








#endif // LAB471_CAMERA_H_INCLUDED