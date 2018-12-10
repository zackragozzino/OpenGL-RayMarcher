#version 330 core
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

const int MAX_MARCHING_STEPS = 155;
const float MIN_DIST = 3000;
const float MAX_DIST = 500.0;
const float EPSILON = 0.0001;

vec3 eye;
vec3 debugColor = vec3(0,0,0);
vec3 offset;

float musicSpeed;

#define Scale 2.0
// Decrease this for better performance
#define Iterations 12
#define normalDistance 0.0002
#define Ambient 0.28452
#define Jitter 0.05

#define Ambient 0.28452
#define Diffuse 0.57378
#define Specular 0.07272
#define LightDir vec3(1.0,1.0,-0.65048)
#define LightColor vec3(1.0,0.666667,0.0)
#define LightDir2 vec3(1.0,-0.62886,1.0)
#define LightColor2 vec3(0.596078,0.635294,1.0)

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

vec2 rotate(vec2 a, float b)
{
    float c = cos(b);
    float s = sin(b);
    return vec2(
        a.x * c - a.y * s,
        a.x * s + a.y * c
    );
}

float sdPlane( vec3 p )
{
	return p.y;
}

float intersectSDF(float distA, float distB) {
    return max(distA, distB);
}

float unionSDF(float distA, float distB) {
    return min(distA, distB);
}

float differenceSDF(float distA, float distB) {
    return max(distA, -distB);
}

float boxSDF(vec3 p, vec3 size) {
    //p = vec3(mod(p.x + 2, 4) - 2, p.y, mod(p.z + 2, 4) - 2);
    vec3 d = abs(p) - (size / 2.0);
    
    // Assuming p is inside the cube, how far is it from the surface?
    // Result will be negative or zero.
    float insideDistance = min(max(d.x, max(d.y, d.z)), 0.0);
    
    // Assuming p is outside the cube, how far is it from the surface?
    // Result will be positive or zero.
    float outsideDistance = length(max(d, 0.0));
    
    return insideDistance + outsideDistance;
}

float sphereSDF(vec3 p, float r) {
    //p = vec3(mod(p.x + 2, 4) - 2, p.y, mod(p.z + 2, 4) - 2);
    return length(vec3(mod(p.x, 0), p.y, p.z)) - r;
}

float cylinderSDF(vec3 p, float h, float r) {
    //p = vec3(mod(p.x + 2, 4) - 2, p.y, mod(p.z + 2, 4) - 2);
    // How far inside or outside the cylinder the point is, radially
    float inOutRadius = length(p.xy) - r;
    
    // How far inside or outside the cylinder is, axially aligned with the cylinder
    float inOutHeight = abs(p.z) - h/2.0;
    
    // Assuming p is inside the cylinder, how far is it from the surface?
    // Result will be negative or zero.
    float insideDistance = min(max(inOutRadius, inOutHeight), 0.0);

    // Assuming p is outside the cylinder, how far is it from the surface?
    // Result will be positive or zero.
    float outsideDistance = length(max(vec2(inOutRadius, inOutHeight), 0.0));
    
    return insideDistance + outsideDistance;
}

// Geometric orbit trap. Creates the 'cube' look.
float trap(vec3 p){
	return  length(p.x-0.5-0.5*sin(iTime/10.0)); // <- cube forms 
	//return  length(p.x-1.0); 
	//return length(p.xz-vec2(1.0,1.0))-0.05; // <- tube forms
	//return length(p); // <- no trap
}

float DE(in vec3 z)
{	
    z = vec3(mod(z.x + 2, 4) - 2, z.y, mod(z.z + 2, 4) - 2);

	// Folding 'tiling' of 3D space;
	z  = abs(1.0-mod(z,2.0));
	
	float d = 1000.0;
	float r;
	for (int n = 0; n < Iterations; n++) {
		z.xz = rotate(z.xz, musicSpeed/18.0);
		
		// This is octahedral symmetry,
		// with some 'abs' functions thrown in for good measure.
		if (z.x+z.y<0.0) z.xy = -z.yx;
		z = abs(z);
		if (z.x+z.z<0.0) z.xz = -z.zx;
		z = abs(z);
		if (z.x-z.y<0.0) z.xy = z.yx;
		z = abs(z);
		if (z.x-z.z<0.0) z.xz = z.zx;
		z = z*Scale - offset*(Scale-1.0);
		z.yz = rotate(z.yz, -musicSpeed/18.0);
		
		d = min(d, trap(z) * pow(Scale, -float(n+1)));
	}
	return d;
}


float sceneSDF(vec3 samplePoint) {    
    // Slowly spin the whole scene
    //samplePoint = rotateY(iTime / 2.0) * samplePoint;
    //samplePoint = vec3(mod(samplePoint.x + 2, 4) - 2, samplePoint.y - fft_buff[0] * 2, mod(samplePoint.z + 2, 4) - 2);
    //samplePoint = vec3(mod(samplePoint.x + 2, 4) - 2, mod(samplePoint.y + 2, 4) - 2, mod(samplePoint.z + 2, 4) - 2);
    //samplePoint.xy = rotate(samplePoint.xy, samplePoint.z*.05);
    
    //vec3 eye = vec3(8.0, 5.0, 7.0);
    float heightVal = 0;
    float intervalDistance = 25;

    vec3 camPos = vec3(eye.x, 5, eye.z);

    float rayDist = distance(camPos, samplePoint);
    int level = 0;

    for(int i = 0; i < 10; i++){
        if(rayDist <= (i+1) * intervalDistance){
            heightVal = fft_buff[i];
            debugColor.r += (i+1) * 0.2;
            level = i;
            break;
        }
    }
    
    switch(level){
        case 0: debugColor = vec3(1,0,0); break;
        case 1: debugColor = vec3(0,1,0); break;
        case 2: debugColor = vec3(0,0,1); break;
        case 3: debugColor = vec3(1,1,0); break;
        case 4: debugColor = vec3(0,1,1); break;
        default: debugColor = vec3(1,0,1); break;
    }

    samplePoint = vec3(mod(samplePoint.x + 2, 4) - 2, samplePoint.y - (heightVal * 5), mod(samplePoint.z + 2, 4) - 2);
    //samplePoint = vec3(mod(samplePoint.x + 2, 4) - 2, mod(samplePoint.y + 2, 24) - (heightVal * 5), mod(samplePoint.z + 2, 4) - 2);

    float cylinderRadius = 0.4 + (1.0 - 0.4) * (1.0 + sin(1.7 * iTime)) / 2.0;
    float cylinder1 = cylinderSDF(samplePoint, 2.0, cylinderRadius);
    float cylinder2 = cylinderSDF(rotateX(radians(90.0)) * samplePoint, 2.0, cylinderRadius);
    float cylinder3 = cylinderSDF(rotateY(radians(90.0)) * samplePoint, 2.0, cylinderRadius);
    
    float cube = boxSDF(samplePoint, vec3(1.8, 1.8, 1.8));
    
    float sphere = sphereSDF(samplePoint, 1.2);
    
    float ballOffset = 0.4 + 1.0 + sin(1.7 * iTime);
    float ballRadius = 0.3;
    float balls = sphereSDF(samplePoint - vec3(ballOffset, 0.0, 0.0), ballRadius);
    balls = unionSDF(balls, sphereSDF(samplePoint + vec3(ballOffset, 0.0, 0.0), ballRadius));
    balls = unionSDF(balls, sphereSDF(samplePoint - vec3(0.0, ballOffset, 0.0), ballRadius));
    balls = unionSDF(balls, sphereSDF(samplePoint + vec3(0.0, ballOffset, 0.0), ballRadius));
    balls = unionSDF(balls, sphereSDF(samplePoint - vec3(0.0, 0.0, ballOffset), ballRadius));
    balls = unionSDF(balls, sphereSDF(samplePoint + vec3(0.0, 0.0, ballOffset), ballRadius));
    
    
    
    float csgNut = differenceSDF(intersectSDF(cube, sphere),
                         unionSDF(cylinder1, unionSDF(cylinder2, cylinder3)));
    float dist = DE(samplePoint);
    //return unionSDF(balls, csgNut);
    return sphere;
}

float shortestDistanceToSurface(vec3 eye, vec3 marchingDirection, float start, float end) {
    float depth = start;
    for (int i = 0; i < MAX_MARCHING_STEPS; i++) {
        float dist = sceneSDF(eye + depth * marchingDirection);
        //float dist = DE(eye + depth * marchingDirection);
        if (dist < EPSILON) {
			return depth;
        }
        depth += dist;
        if (depth >= end) {
            return end;
        }
    }
    
    return end;
}

// Finite difference normal
vec3 getNormal(in vec3 pos) {
	vec3 e = vec3(0.0,normalDistance,0.0);
    //return e; // ????

	return normalize(vec3(
			DE(pos+e.yxx)-DE(pos-e.yxx),
			DE(pos+e.xyx)-DE(pos-e.xyx),
			DE(pos+e.xxy)-DE(pos-e.xxy)));
}

// Two light source + env light
vec3 getLight(in vec3 color, in vec3 normal, in vec3 dir) {
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

// Solid color with a little bit of normal :-)
vec3 getColor(vec3 normal, vec3 pos, float dist) {
    vec3 musicColor = vec3(abs(sin(musicSpeed * 0.2 + dist)), abs(cos(musicSpeed*0.2 + dist)), abs(sin(iTime * 0.2)));
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
    float depth = start;
    float totalDistance = Jitter*rand(fragCoord+vec2(iTime));
    
    float distance;
	int steps = 0;
	vec3 pos;

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

vec3 estimateNormal(vec3 p) {
    return normalize(vec3(
        sceneSDF(vec3(p.x + EPSILON, p.y, p.z)) - sceneSDF(vec3(p.x - EPSILON, p.y, p.z)),
        sceneSDF(vec3(p.x, p.y + EPSILON, p.z)) - sceneSDF(vec3(p.x, p.y - EPSILON, p.z)),
        sceneSDF(vec3(p.x, p.y, p.z  + EPSILON)) - sceneSDF(vec3(p.x, p.y, p.z - EPSILON))
    ));
}

vec3 phongContribForLight(vec3 k_d, vec3 k_s, float alpha, vec3 p, vec3 eye,
                          vec3 lightPos, vec3 lightIntensity) {
    vec3 N = estimateNormal(p);
    vec3 L = normalize(lightPos - p);
    vec3 V = normalize(eye - p);
    vec3 R = normalize(reflect(-L, N));
    
    float dotLN = dot(L, N);
    float dotRV = dot(R, V);
    
    if (dotLN < 0.0) {
        // Light not visible from this point on the surface
        return vec3(0.0, 0.0, 0.0);
    } 
    
    if (dotRV < 0.0) {
        // Light reflection in opposite direction as viewer, apply only diffuse
        // component
        return lightIntensity * (k_d * dotLN);
    }
    return lightIntensity * (k_d * dotLN + k_s * pow(dotRV, alpha));
}

vec3 phongIllumination(vec3 k_a, vec3 k_d, vec3 k_s, float alpha, vec3 p, vec3 eye) {
    const vec3 ambientLight = 0.5 * vec3(1.0, 1.0, 1.0);
    vec3 color = ambientLight * k_a;
    
    vec3 light1Pos = vec3(4.0 * sin(iTime),
                          2.0,
                          4.0 * cos(iTime));
    vec3 light1Intensity = vec3(0.4, 0.4, 0.4);
    
    color += phongContribForLight(k_d, k_s, alpha, p, eye,
                                  light1Pos,
                                  light1Intensity);
    
    vec3 light2Pos = vec3(2.0 * sin(0.37 * iTime),
                          2.0 * cos(0.37 * iTime),
                          2.0);
    vec3 light2Intensity = vec3(0.4, 0.4, 0.4);
    
    color += phongContribForLight(k_d, k_s, alpha, p, eye,
                                  light2Pos,
                                  light2Intensity);    
    return color;
}

mat3 viewMatrix(vec3 eye, vec3 center, vec3 up) {
    // Based on gluLookAt man page
    vec3 cameraDirection = normalize(center - eye);
    vec3 cameraRight = normalize(cross(cameraDirection, up));
    vec3 cameraUp = cross(cameraRight, cameraDirection);
    return mat3(cameraRight, cameraUp, -cameraDirection);
}

void main( )
{
    float slowTime = iTime * 0.2;
	vec3 viewDir = rayDirection(45.0, iResolution.xy, fragCoord);
    //vec3 eye = vec3(8.0, 5.0 * sin(0.2 * iTime), 7.0);
    float speed = iTime * 5 * fft_buff[0];
    //eye = vec3(8.0, 5.0, 7.0 + vizSpeed * 2 + iTime);
    //eye = vec3(8.0, 5.0 - slowTime * 0.5, 7.0);

    musicSpeed = iTime * 0.5 + vizSpeed * 0.5;
    
    //eye = vec3(8.0, 5.0 - slowTime * 0.5 + vizSpeed * 0.1, 7.0);
    //eye = vec3(0, 0, - (slowTime + vizSpeed * 0.1));
    //eye = vec3(34,0,2);
    eye += campos;
    //eye += vec3(-2.04, -15.92, -1.01);

    mat3 viewToWorld;
    if(VR_Enabled == 1){
        viewToWorld = mat3(vec3(-VRdir[0][0], VRdir[0][1], VRdir[0][2]),
                       vec3(VRdir[1][0], -VRdir[1][1], VRdir[1][2]),
                       vec3(VRdir[2][0], VRdir[2][1], -VRdir[2][2]) );
    }
    else
        viewToWorld = viewMatrix(eye, eye + cameraFront, vec3(0.0, 1.0, 0.0));

    vec3 worldDir = viewToWorld * viewDir.xyz;
    //worldDir.y *= -1;

    bool fractal = true;
    if(!fractal){
        float dist = shortestDistanceToSurface(eye, worldDir, MIN_DIST, MAX_DIST);
    
        if (dist > MAX_DIST - EPSILON) {
            // Didn't hit anything
            fragColor = vec4(0.0, 0.0, 0.0, 0.0);
		    return;
        }
    
        // The closest point on the surface to the eyepoint along the view ray
        vec3 p = eye + dist * worldDir;
    
        // Use the surface normal as the ambient color of the material
        vec3 K_a = (estimateNormal(p) + vec3(1.0)) / 2.0;
        vec3 K_d = K_a;
        vec3 K_s = vec3(1.0, 1.0, 1.0);
        float shininess = 10.0;
    
        vec3 color = phongIllumination(K_a, K_d, K_s, shininess, p, eye);
        color.r = fft_buff[0];
        color.g = fft_buff[1];
        color.b = fft_buff[2];
        color = debugColor;
        fragColor = vec4(color, 1.0);
    }
    else{
        offset = vec3(1.0+0.2*(cos(iTime/5.7)),0.3+0.1*(cos(iTime/1.7)),1.).xzy;;

        vec2 coord =-1.0+2.0*fragCoord.xy/iResolution.xy;
	    coord.x *= iResolution.x/iResolution.y;

    	vec3 camUp  = vec3(0,1,0);
	    vec3 camRight = normalize(cross(eye,camUp));
	    // Get direction for this pixel

        //vec3 camPos = vec3(1.0,0.0,0.0);

        vec3 camDir   = normalize(eye+cameraFront); // direction for center ray
	
	    // Get direction for this pixel
	    vec3 rayDir = normalize(camDir + (coord.x*camRight + coord.y*camUp));

        fragColor = raymarch(eye, worldDir, MIN_DIST);
    }
}