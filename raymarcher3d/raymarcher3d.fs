#version 430

out vec4 FragColor;
in vec2 texCoord;

#define SHAPE_TYPE_SPHERE 0
#define SHAPE_TYPE_BOX 1
#define MAX_SHAPES 64

struct Shape{
    int type;
    vec3 origin;
    vec3 values;
};


uniform vec2 iResolution;
uniform float iTime;

uniform int shapeCount;
uniform float k;

uniform int shapeTypes[MAX_SHAPES];
uniform vec3 shapeOrigins[MAX_SHAPES];
uniform vec3 shapeSizes[MAX_SHAPES];

uniform vec3 camOrigin;
uniform vec3 camDir;
uniform float camFOV;
uniform float clipEnd;
uniform float hitThreshold;

Shape shapes[MAX_SHAPES];

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

float smin(float a, float b, float k)
{
    if(k <= 0.0)
    {
        return min(a, b);
    }
    float r = exp2(-a / k) + exp2(-b / k);
    return -k * log2(r);
}

float getSdf(Shape s, vec3 pt)
{
    if(s.type == SHAPE_TYPE_SPHERE)
    {
        return sdSphere(s.origin, pt, s.values.x);
        //return sdBox(s.origin, pt, s.values);
    }
    else if(s.type == SHAPE_TYPE_BOX)
    {
        return sdBox(s.origin, pt, s.values);
    }

    return 1e6;
}

float sceneSDF(vec3 pt)
{
    float d = getSdf(shapes[0], pt);

    for(int i = 1; i < shapeCount; ++i)
    {
        float di = getSdf(shapes[i], pt);
        d = smin(d, di, k);
    }

    return d;
}

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

void main()
{
    // SETUP SHAPES
    for(int i = 0; i < shapeCount; i++)
    {
        shapes[i].origin = shapeOrigins[i];
        shapes[i].type = shapeTypes[i];
        shapes[i].values = shapeSizes[i];
    }
    // DO STUFF
    //FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    //return;

    // FOR EACH PIXEL:
    // - init camera ray
    // - march ray
    // - set color based on ray value

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
    while(totalDistance < clipEnd && length >= hitThreshold)
    {
        length = sceneSDF(origin);

        // march ray once
        origin += dir * length;
        totalDistance += length;
        stepsTaken++;
    }

    bool didHit;
    vec3 color;
    vec4 glow = vec4(0.8, 0.0, 1.0, 1.0) * stepsTaken * 0.02;

    // ray has finished! get ray data and set color
    if (totalDistance > clipEnd)
    {
        // no hit
        didHit = false;
        color = vec3(0.0, 0.0, 0.2);
    } 
    else
    {
        // hit
        didHit = true;
        color = vec3(0.0, 0.0, 0.2);
    }

    FragColor = vec4(color, 1.0) + glow;
}

