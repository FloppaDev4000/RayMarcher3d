#version 330 core
out vec4 FragColor;
in vec2 fragCoord;

#define SHAPE_TYPE_SPHERE 0
#define SHAPE_TYPE_BOX 1
#define MAX_SHAPES 64

struct Shape{
    int type;
    vec3 origin;
    vec3 values;
}


uniform vec2 iResolution;
uniform float iTime;

uniform Shape shapes[MAX_SHAPES];
uniform int shapeCount;
uniform float k;

uniform int shapeTypes[MAX_SHAPES];
uniform vec3 shapeOrigins[MAX_SHAPES];
uniform vec3 shapeSizes[MAX_SHAPES];


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
        float di = getSdf(shapes[i], p);
        d = smin(d, di, k);
    }

    return d;
}