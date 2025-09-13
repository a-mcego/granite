#pragma once

const char*const vertexSourceMain = R"glsl(#version 430
void main()
{
    float x = float(gl_VertexID)-1.0;
    gl_Position = vec4(3.0*x, 2.0-3.0*x*x, 0.0, 1.0);
}
)glsl";

const float SCREEN_X = 1280.0;
const float SCREEN_Y = 960.0;

const char*const fragmentSourceMain = R"glsl(#version 430
out vec4 color;
uniform sampler2D TextureSampler;
uniform float screenSizeX;
uniform float screenSizeY;
#define SCREEN_X %f
#define SCREEN_Y %f
void main()
{
    float texX = %f;
    float texY = %f;
    ivec2 tex_coord = ivec2(gl_FragCoord.x*(texX/SCREEN_X)*(screenSizeX/texX), (SCREEN_Y-gl_FragCoord.y)*(texY/SCREEN_Y)*(screenSizeY/texY));
    color.rgb = texelFetch(TextureSampler, tex_coord, 0).rgb;
    color.a = 1;
}
)glsl";





