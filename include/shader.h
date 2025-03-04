#pragma once

const char* vertexSource = R"glsl(#version 430
void main()
{
    float x = float(gl_VertexID)-1.0;
    gl_Position = vec4(3.0*x, 2.0-3.0*x*x, 0.0, 1.0);
}
)glsl";

//const float SCREEN_X = 1280.0;
//const float SCREEN_Y = 720.0;

const float SCREEN_X = 912.0*1.5;
const float SCREEN_Y = 262.0*3.0;

const char* fragmentSource = R"glsl(#version 430
out vec4 color;
uniform sampler2D TextureSampler;
#define SCREEN_X %f
#define SCREEN_Y %f
void main()
{
    ivec2 tex_coord = ivec2(gl_FragCoord.x*(%f/SCREEN_X), (SCREEN_Y-gl_FragCoord.y)*(%f/SCREEN_Y));
    color.rgb = texelFetch(TextureSampler, tex_coord, 0).rgb;
    color.a = 1;
}
)glsl";

char infoLog[2048];
GLuint compileShader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shader, 2048, nullptr, infoLog);
        std::cerr << "Shader compilation failed: " << infoLog << std::endl;
        std::abort();
    }

    return shader;
}

GLuint createShaderProgram(const char* vertexSource, const char* fragmentSource, u32 X, u32 Y)
{
    char processed_fragment_source[4096] = {};
    sprintf(processed_fragment_source, fragmentSource, SCREEN_X, SCREEN_Y, float(X), float(Y));

    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, processed_fragment_source);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    GLint success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 2048, nullptr, infoLog);
        std::cerr << "Shader linking failed: " << infoLog << std::endl;
        std::abort();
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}



