#version 330 core

// based on :
// https://www.shadertoy.com/view/XsX3z7#
// Created by Mikael Hvidtfeldt Christensen
// @SyntopiaDK

out vec4 fragColor;

in vec2 fragCoord;

uniform vec3 campos;
uniform vec3 cameraFront;
uniform float iTime;
uniform float vizSpeed;
uniform vec2 iResolution;
uniform float fft_buff[10];
uniform mat4 VRdir;
uniform int VR_Enabled;


vec3 eye;
vec3 offset;
vec3 LightColor = vec3(1.0,0.7,0.0);
vec3 LightColor2 = vec3(0.6, 0.63,1.0);
float musicSpeed;

// Decrease this for better performance
#define Iterations 12

#define Scale 2.0
#define normalDistance 0.0002
#define Jitter 0.05
#define Ambient 0.3
#define Diffuse 0.6
#define Specular 0.072
#define LightDir vec3(1.0,1.0,-0.65)
#define LightDir2 vec3(1.0,-0.62886,1.0)
#define EPSILON 0.0001
#define MIN_DIST 30
#define MAX_MARCHING_STEPS 155

mat3 rotateX(float theta) {
    float c = cos(theta);
    float s = sin(theta);
    return mat3(
        vec3(1, 0, 0),
        vec3(0, c, -s),
        vec3(0, s, c)
    );
}

mat3 rotateY(float theta) {
    float c = cos(theta);
    float s = sin(theta);
    return mat3(
        vec3(c, 0, s),
        vec3(0, 1, 0),
        vec3(-s, 0, c)
    );
}

mat3 rotateZ(float theta) {
    float c = cos(theta);
    float s = sin(theta);
    return mat3(
        vec3(c, -s, 0),
        vec3(s, c, 0),
        vec3(0, 0, 1)
    );
}

vec2 rotate(vec2 v, float a) {
	return vec2(cos(a)*v.x + sin(a)*v.y, -sin(a)*v.x + cos(a)*v.y);
}

//http://www.fractalforums.com/sierpinski-gasket/kaleidoscopic-(escape-time-ifs)/
float DE(in vec3 z)
{	
	// Folding 'tiling' of 3D space;
	z  = abs(1.0-mod(z,2.0));

	float d = 1000.0;
	for (int n = 0; n < Iterations; n++) {
		z.xz = rotate(z.xz, musicSpeed/18.0);
        z.xy = rotate(z.xy,4.0+cos( musicSpeed/18.0));
		
		if (z.x+z.y<0.0)
            z.xy = -z.yx;
		z = abs(z);

		if (z.x+z.z<0.0) 
            z.xz = -z.zx;
		z = abs(z);
		
        if (z.x-z.y<0.0) 
            z.xy = z.yx;
		z = abs(z);
		
        if (z.x-z.z<0.0) 
            z.xz = z.zx;
		
        z = z*Scale - offset*(Scale-1.0);
		z.yz = rotate(z.yz, -musicSpeed/18.0);

        float trap = length(z.x-0.5-0.5*sin(musicSpeed/10.0));
		
		d = min(d, trap * pow(Scale, -float(n+1)));
	}
	return d;
}

vec3 getNormal(vec3 p) {
    return normalize(vec3(
        DE(vec3(p.x + EPSILON, p.y, p.z)) - DE(vec3(p.x - EPSILON, p.y, p.z)),
        DE(vec3(p.x, p.y + EPSILON, p.z)) - DE(vec3(p.x, p.y - EPSILON, p.z)),
        DE(vec3(p.x, p.y, p.z  + EPSILON)) - DE(vec3(p.x, p.y, p.z - EPSILON))
    ));
}


vec3 getLight(in vec3 color, in vec3 normal, in vec3 dir) {
    //LightColor = vec3(abs(sin(musicSpeed * 0.2)), abs(cos(musicSpeed*0.2)), abs(sin(iTime * 0.2)));

	vec3 lightDir = normalize(LightDir);
	float specular = pow(max(0.0,dot(lightDir,-reflect(lightDir, normal))),20.0); // Phong
	float diffuse = max(0.0,dot(-normal, lightDir)); // Lambertian
	
	vec3 lightDir2 = normalize(LightDir2);
	float specular2 = pow(max(0.0,dot(lightDir2,-reflect(lightDir2, normal))),20.0); // Phong
	float diffuse2 = max(0.0,dot(-normal, lightDir2)); // Lambertian
	
	return
		(Specular*specular)*LightColor+(diffuse*Diffuse)*(LightColor*color) +
		(Specular*specular2)*LightColor2+(diffuse2*Diffuse)*(LightColor2*color);
}

vec3 getColor(vec3 normal, vec3 pos, float dist) {
    vec3 musicColor = vec3(abs(sin(musicSpeed * 0.2 + dist)), abs(cos(musicSpeed*0.2 + dist)), abs(sin(musicSpeed * 0.2)));
    musicColor += dot(musicColor, normal);
	return mix(musicColor,abs(normal),0.3);
}

vec3 toneMap(in vec3 c) {
	c=pow(c,vec3(2.0));
	vec3 x = max(vec3(0.),c-vec3(0.004));
	c = (x*(6.2*x+.5))/(x*(6.2*x+1.7)+0.06);
	return c;
}

float rand(vec2 co){
	return fract(cos(dot(co,vec2(4.898,7.23))) * 23421.631);
}

vec4 raymarch(vec3 eye, vec3 marchingDirection, float start) {
    float distance;
    vec3 pos;
	int steps = 0;
    float depth = start;
    float totalDistance = Jitter*rand(fragCoord+vec2(iTime));

    //Standard raymarching algorithm
    for (int i=0; i < MAX_MARCHING_STEPS; i++) {
		pos = eye + totalDistance * marchingDirection;
		distance = DE(pos);
		totalDistance += distance;
		if (distance < EPSILON) break;
		steps = i;
	}

    float smoothStep = float(steps) + distance/EPSILON;
    float ao = 1.0-smoothStep/float(MAX_MARCHING_STEPS);

    vec3 normal = getNormal(pos-marchingDirection*normalDistance*3.0);	
	vec3 color = getColor(normal, pos, totalDistance);	
    vec3 light = getLight(color, normal, marchingDirection);

	return vec4(toneMap((color*Ambient+light)*ao),1.0);
}
           
vec3 rayDirection(float fieldOfView, vec2 size, vec2 fragCoord) {
    vec2 xy = fragCoord - size / 2.0;
    float z = size.y / tan(radians(fieldOfView) / 2.0);
    return normalize(vec3(xy, -z));
} 

mat3 generateViewMatrix(vec3 eye, vec3 center, vec3 up) {
    // Based on gluLookAt
    vec3 cameraDirection = normalize(center - eye);
    vec3 cameraRight = normalize(cross(cameraDirection, up));
    vec3 cameraUp = cross(cameraRight, cameraDirection);
    return mat3(cameraRight, cameraUp, -cameraDirection);
}

void main( )
{
    mat3 viewMatrix;
    float slowTime = iTime * 0.2;
	vec3 viewDir = rayDirection(45.0, iResolution.xy, fragCoord);
    //vec3 eye = vec3(8.0, 5.0 * sin(0.2 * iTime), 7.0);
    float speed = iTime * 5 * fft_buff[0];
    //eye = vec3(8.0, 5.0, 7.0 + vizSpeed * 2 + iTime);
    //eye = vec3(8.0, 5.0 - slowTime * 0.5, 7.0);

    musicSpeed = iTime * 0.5 + vizSpeed * 0.5;
    
    //eye = vec3(8.0, 5.0 - slowTime * 0.5 + vizSpeed * 0.1, 7.0);
    //eye = vec3(0, 0, - (slowTime + vizSpeed * 0.05));
    //eye = vec3(34,0,2);
    //eye.z -= iTime;
    eye += campos;
    //eye += vec3(-2.04, -15.92, -1.01);

    //If in VR, use the provided view matrix
    if(VR_Enabled == 1){
        viewMatrix = mat3(vec3(-VRdir[0][0], VRdir[0][1], VRdir[0][2]),
                           vec3(VRdir[1][0], -VRdir[1][1], VRdir[1][2]),
                           vec3(VRdir[2][0], VRdir[2][1], -VRdir[2][2]) );
    }
    //Otherwise generate our own
    else{
        viewMatrix = generateViewMatrix(eye, eye + cameraFront, vec3(0.0, 1.0, 0.0));
    }

    vec3 worldDir = viewMatrix * viewDir.xyz;

    offset = vec3(1.0+0.2*(cos(iTime/5.7)),0.3+0.1*(cos(iTime/1.7)),1.).xzy;

    vec2 coord =-1.0+2.0*fragCoord.xy/iResolution.xy;
	coord.x *= iResolution.x/iResolution.y;

    vec3 camUp  = vec3(0,1,0);
	vec3 camRight = normalize(cross(eye,camUp));
	// Get direction for this pixel

    vec3 camDir   = normalize(eye+cameraFront); // direction for center ray
	
	// Get direction for this pixel
	vec3 rayDir = normalize(camDir + (coord.x*camRight + coord.y*camUp));

    fragColor = raymarch(eye, worldDir, MIN_DIST);
}