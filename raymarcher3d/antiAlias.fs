#version 430 core

in vec2 fragTexCoord;
out vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;

#define FXAA_REDUCE_MIN   (1.0/128.0)
#define FXAA_REDUCE_MUL   (1.0/8.0)
#define FXAA_SPAN_MAX     8.0

void main()
{
    vec2 texel = 1.0 / resolution;
    vec3 rgbNW = texture(texture0, fragTexCoord + texel * vec2(-1.0, -1.0)).rgb;
    vec3 rgbNE = texture(texture0, fragTexCoord + texel * vec2(1.0, -1.0)).rgb;
    vec3 rgbSW = texture(texture0, fragTexCoord + texel * vec2(-1.0, 1.0)).rgb;
    vec3 rgbSE = texture(texture0, fragTexCoord + texel * vec2(1.0, 1.0)).rgb;
    vec3 rgbM  = texture(texture0, fragTexCoord).rgb;

    // Luma (perceived brightness)
    float lumaNW = dot(rgbNW, vec3(0.299, 0.587, 0.114));
    float lumaNE = dot(rgbNE, vec3(0.299, 0.587, 0.114));
    float lumaSW = dot(rgbSW, vec3(0.299, 0.587, 0.114));
    float lumaSE = dot(rgbSE, vec3(0.299, 0.587, 0.114));
    float lumaM  = dot(rgbM,  vec3(0.299, 0.587, 0.114));

    // Compute gradients
    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);

    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);

    dir = clamp(dir * rcpDirMin, vec2(-FXAA_SPAN_MAX), vec2(FXAA_SPAN_MAX)) * texel;

    vec3 rgbA = 0.5 * (
        texture(texture0, fragTexCoord + dir * (1.0 / 3.0 - 0.5)).rgb +
        texture(texture0, fragTexCoord + dir * (2.0 / 3.0 - 0.5)).rgb );

    vec3 rgbB = rgbA * 0.5 + 0.25 * (
        texture(texture0, fragTexCoord + dir * -0.5).rgb +
        texture(texture0, fragTexCoord + dir * 0.5).rgb );

    float lumaB = dot(rgbB, vec3(0.299, 0.587, 0.114));

    if (lumaB < lumaMin || lumaB > lumaMax)
        fragColor = vec4(rgbA, 1.0);
    else
        fragColor = vec4(rgbB, 1.0);
}
