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

vec3 lightColor = vec3(0.47, 0.89, 1.0);
float shininess = 22;

uniform vec2 iResolution;
uniform float iTime;

uniform vec3 lightPos;

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

vec3 getNormal(vec3 pt)
{
    const float EPS = 0.001;

    vec3 v1 = vec3(
        sceneSDF(pt + vec3(EPS, 0.0, 0.0)),
        sceneSDF(pt + vec3(0.0, EPS, 0.0)),
        sceneSDF(pt + vec3(0.0, 0.0, EPS))
    );
    vec3 v2 = vec3(
        sceneSDF(pt - vec3(EPS, 0.0, 0.0)),
        sceneSDF(pt - vec3(0.0, EPS, 0.0)),
        sceneSDF(pt - vec3(0.0, 0.0, EPS))
    );

    return normalize(v1 - v2);
}

float softShadow(vec3 rayOrigin, vec3 rayDir, float start, float end, float k)
{
    float result = 1.0;
    float prevDistance = 1e20;

    for(float distTravelled = start; distTravelled < end;)
    {
        //float distToScene = sceneSDF(rayOrigin + rayDir * distTravelled).w;
    }

    return result;
}

float shadow( in vec3 ro, in vec3 rd, float mint, float maxt )
{
    float t = mint;
    for( int i=0; i<256 && t<maxt; i++ )
    {
        float h = sceneSDF(ro + rd*t);
        if( h<0.001 )
            return 0.0;
        t += h;
    }
    return 1.0;
}

vec3 lightDir(vec3 pos)
{
    return normalize(lightPos - pos);
}

// PHONG
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
        color = vec3(1.0, 0.0, 0.2);
        
        vec3 ambient = phongAmbient();
        vec3 diffuse = phongDiffuse(getNormal(origin), origin);
        vec3 specular = phongSpecular(getNormal(origin), origin);

        vec3 lighting = ambient + diffuse + specular;

        color *= lighting;
    }

    FragColor = vec4(color, 1.0) + glow;
}

