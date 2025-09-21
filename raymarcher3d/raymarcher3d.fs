#version 430

out vec4 FragColor;
in vec2 texCoord;

#define SHAPE_TYPE_SPHERE 0
#define SHAPE_TYPE_BOX 1
#define SHAPE_TYPE_TORUS 2
#define SHAPE_TYPE_MANDELBULB 3

#define MAX_SHAPES 64

struct Shape{
    int type;
    vec3 origin;
    vec3 values;
};

uniform vec2 iResolution;
uniform float iTime;

uniform int shapeCount;

uniform int shapeTypes[MAX_SHAPES];
uniform vec3 shapeOrigins[MAX_SHAPES];
uniform vec3 shapeSizes[MAX_SHAPES];
uniform vec3 shapeCols[MAX_SHAPES];

uniform vec3 camOrigin;
uniform vec3 camDir;
uniform float camFOV;
uniform float clipEnd;
uniform float hitThreshold;

// debug parameters
uniform vec3 lightPos;
uniform float sb;
uniform float k;
uniform vec3 lightColor;
uniform vec3 bgColor;
uniform float shininess;
uniform vec3 glowCol;
uniform float glowIntensity;
uniform float shadowSmoothness;

float shadowBias = hitThreshold * sb;

Shape shapes[MAX_SHAPES];

//--------------------------------------SHAPE SDFS

float sdSphere(vec3 origin, vec3 pt, float radius)
{
    vec3 p = pt + origin;
    return length(p) - radius;
}

float sdBox(vec3 origin, vec3 pt, vec3 box)
{
    vec3 p = pt + origin;
    vec3 q = abs(p) - box;
    return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

float sdTorus(vec3 origin, vec3 pt, vec2 torus)
{
    vec3 p = pt + origin;
    vec2 q = vec2(length(p.xz)-torus.x,p.y);
    return length(q)-torus.y;
}

float sdfMandelbulb(vec3 origin, vec3 pt, vec3 data)
{
    vec3 p = pt + origin;

    float iterations = data.x;
    float radius = data.y;
    float power = data.z;

    vec3 z = p;
    float dr = 1.0;
    float r = 0.0;

    for(int i = 0; i < iterations; i++)
    {
        r = length(z);
        if(r > radius)
        {
            break;
        }
        float theta = acos(z.y/r);
        float phi = atan(z.z, z.x);
        dr = pow(r, power - 1.0) * power * dr + 1.0;

        float zr = pow(r, power);
        theta = theta * power;
        phi = phi * power;

        z = zr * vec3(sin(theta) * cos(phi), cos(theta), sin(phi)*sin(theta));
        z += p;
    }

    return 0.5 * log(r) * r / dr;
}

vec2 smin( float a, float b, float k )
{
    float h = 1.0 - min( abs(a-b)/(4.0*k), 1.0 );
    float w = h*h;
    float m = w*0.5;
    float s = w*k;
    return (a<b) ? vec2(a-s,m) : vec2(b-s,1.0-m);
}

vec4 combine(float dstA, float dstB, vec3 colA, vec3 colB, int operation, float k)
{
    float dst = dstA;
    vec3 col = colA;

    if(operation == 0) // union
    {
        vec2 sminData = smin(dstA, dstB, k);
        dst = sminData.x;
        col = mix(colA, colB, sminData.y);
    }

    return vec4(col, dst);
}

float getSdf(Shape s, vec3 pt)
{
    if(s.type == SHAPE_TYPE_SPHERE)
    {
        return sdSphere(s.origin, pt, s.values.x);
    }
    else if(s.type == SHAPE_TYPE_BOX)
    {
        return sdBox(s.origin, pt, s.values);
    }
    else if(s.type == SHAPE_TYPE_TORUS)
    {
        return sdTorus(s.origin, pt, s.values.xy);
    }
    else if(s.type == SHAPE_TYPE_MANDELBULB)
    {
        return sdfMandelbulb(s.origin, pt, s.values);
    }

    return 1e6;
}

vec4 sceneSDF(vec3 pt)
{
    float totalDist = 1e6;
    vec3 totalCol = vec3(0, 0, 0);
    for(int i = 0; i < shapeCount; ++i)
    {
        float distance = getSdf(shapes[i], pt);
        vec3 col = shapeCols[i];
        vec4 data = combine(totalDist, distance, totalCol, col, 0, k);
        totalCol = data.xyz;
        totalDist = data.w;
    }

    return vec4(totalCol, totalDist);
}

vec3 getNormal(vec3 pt)
{
    const float EPS = 0.001;

    vec3 v1 = vec3(
        sceneSDF(pt + vec3(EPS, 0.0, 0.0)).w,
        sceneSDF(pt + vec3(0.0, EPS, 0.0)).w,
        sceneSDF(pt + vec3(0.0, 0.0, EPS)).w
    );
    vec3 v2 = vec3(
        sceneSDF(pt - vec3(EPS, 0.0, 0.0)).w,
        sceneSDF(pt - vec3(0.0, EPS, 0.0)).w,
        sceneSDF(pt - vec3(0.0, 0.0, EPS)).w
    );

    return normalize(v1 - v2);
}

float softShadow(vec3 origin, vec3 lightDir, float minT, float maxT)
{
    origin = origin + getNormal(origin) * minT;
    float res = 1.0;
    float t = minT;

    for (int i = 0; i < 64; ++i) {
        float h = sceneSDF(origin + lightDir * t).w;
        if (h < 0.001) {
            return 0.0; // fully in shadow
        }
        res = min(res, shadowSmoothness * h / t);
        t += clamp(h, 0.01, 0.5); // step size
        if (t > maxT) break;
    }

    return clamp(res, 0.0, 1.0);
}

float ambientOcclusion (vec3 pos, vec3 normal)
{
    float sum = 0;
    int aoSteps = 5;
    float aoStepSize = 0.1;
    for (int i = 0; i < aoSteps; i ++)
    {
        vec3 p = pos + normal * (i+1) * aoStepSize;
        sum += sceneSDF(p).w;
    }
    return sum / (aoSteps * aoStepSize);

    float acc = 0;
    for(int i = 1; i <= aoSteps; ++i)
    {
        float d = sceneSDF(pos + i * aoStepSize * normal).w;
        acc += exp2(-i) * (i*aoStepSize - max(d, 0));
    }
    return min(1 - 0.5 * acc, 1);
}

vec3 lightDir(vec3 pos)
{
    return normalize(lightPos - pos);
}

float saturate(float f)
{
    return clamp(f, 0.0, 1.0);
}

//--------------------------------------PHONG
vec3 phongAmbient()
{
    return 0.05 * lightColor;
}
vec3 phongDiffuse(vec3 normal, vec3 pos)
{
    float dif = max(dot(normalize(normal), lightDir(pos)), 0.0);
    return dif * lightColor;
}
vec3 phongSpecular(vec3 normal, vec3 pos)
{
    vec3 reflectDir = reflect(lightDir(pos), normal);
    float spec = pow(max(dot(camDir, reflectDir), 0.0), shininess);
    return spec * lightColor;
}

//--------------------------------------BLEND MODES
vec3 multiply(vec3 base, vec3 multiply, float opacity)
{
    return base * multiply * opacity + base * (1.0 - opacity);
}

//--------------------------------------CAMERA DIRS
vec3 worldUp()
{
    return vec3(0.0, 1.0, 0.0);
}

vec3 camForward()
{
    return normalize(camDir);
}
vec3 camRight()
{
    return normalize(cross(worldUp(), camForward()));
}
vec3 camUp()
{
    return cross(camForward(), camRight());
}


//--------------------------------------MAIN
void main()
{
    // SETUP SHAPES
    for(int i = 0; i < shapeCount; i++)
    {
        shapes[i].origin = shapeOrigins[i];
        shapes[i].type = shapeTypes[i];
        shapes[i].values = shapeSizes[i];
    }

    // INIT RAY
    float aspect = iResolution.x / iResolution.y;
    float halfHeight = tan(camFOV / 2.0f);
    float halfWidth = aspect * halfHeight;

    float u = (gl_FragCoord.x + 0.5f) / iResolution.x;
    float v = (gl_FragCoord.y + 0.5f) / iResolution.y;

    float x = (2.0f * u - 1.0f) * halfWidth;
    float y = (1.0f - 2.0f * v) * halfHeight;


    // init ray values
    vec3 dir = normalize(camForward() + (camRight() * x) + (camUp() * y));
    vec3 origin = camOrigin;
    float totalDistance = 0.0;
    int stepsTaken = 0;

    // MARCH RAY
    float length = hitThreshold;
    vec3 localCol = vec3(0.0, 0.0, 0.0);
    vec4 info;
    while(totalDistance < clipEnd && length >= hitThreshold)
    {
        info = sceneSDF(origin);
        length = info.w;

        // march ray once
        origin += dir * length;
        totalDistance += length;
        stepsTaken++;
    }

    vec3 color;
    vec4 glow = vec4(glowCol, 1.0) * stepsTaken * glowIntensity;

    // ray has finished! get ray data and set color
    if (totalDistance > clipEnd)
    {
        // no hit
        color = bgColor;
    } 
    else
    {
        // hit
        color = info.xyz;
        vec3 normal = getNormal(origin);
        

        // SHADING DATA
        float lighting = saturate(dot(normal, lightDir(origin)));

        vec3 offsetPos = origin + normal * shadowBias;
        vec3 shadowLightDir = lightDir(offsetPos);
        float dstToLight = distance(offsetPos, lightPos);
        float shadow = softShadow(offsetPos, shadowLightDir, shadowBias, dstToLight);

        vec3 ambient = phongAmbient();

        vec3 specular = phongSpecular(getNormal(origin), origin);
        
        float AO = ambientOcclusion(origin, normal) * shadow;

        float totalLight = min(lighting, shadow);
        if(totalLight < 0.0){totalLight = 1;}

        totalLight = pow(totalLight, 1/2.4) * 1.055 - 0.055; // gamma correction
        vec3 totalLightVec = vec3(totalLight);// + glow.xyz;

        // final col setup
        color = color*totalLight + bgColor*0.5 + specular*totalLight;
    }

    FragColor = vec4(color, 1);// + glow;
}