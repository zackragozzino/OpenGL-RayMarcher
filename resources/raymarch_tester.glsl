#version 330 core
out vec4 fragColor;

in vec2 fragCoord;

uniform vec3 campos;
uniform vec3 cameraFront;
uniform float iTime;
uniform vec2 iResolution;
uniform float fft_buff[10];

const int MAX_MARCHING_STEPS = 255;
const float MIN_DIST = 0.0;
const float MAX_DIST = 1000.0;
const float EPSILON = 0.0001;

float totalDistance;
vec3 eye;
vec3 debugColor = vec3(0,0,0);

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
    
    return unionSDF(balls, csgNut);
}

float shortestDistanceToSurface(vec3 eye, vec3 marchingDirection, float start, float end) {
    float depth = start;
    for (int i = 0; i < MAX_MARCHING_STEPS; i++) {
        float dist = sceneSDF(eye + depth * marchingDirection);
        if (dist < EPSILON) {
            totalDistance += depth;
			return depth;
        }
        depth += dist;
        if (depth >= end) {
            totalDistance += end;
            return end;
        }
    }
    totalDistance += end;
    return end;
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
	vec3 viewDir = rayDirection(45.0, iResolution.xy, fragCoord);
    //vec3 eye = vec3(8.0, 5.0 * sin(0.2 * iTime), 7.0);
    totalDistance = 0;
    eye = vec3(8.0, 5.0, 7.0 + iTime * 5);
    eye += campos;
    
    mat3 viewToWorld = viewMatrix(eye, eye + cameraFront, vec3(0.0, 1.0, 0.0));
    
    vec3 worldDir = viewToWorld * viewDir.xyz;
    
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
    //color.r = fft_buff[0];
    //color.g = fft_buff[1];
    //color.b = fft_buff[2];
    color = debugColor;
    fragColor = vec4(color, 1.0);
}