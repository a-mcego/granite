#pragma once

#include "shader.h"
#include "palette.h"
#include <ctime>

struct SCREEN
{
    u32 pixels[1024*1025] = {};
    u16 X{1024}, Y{512};
    std::atomic<u32> screenSizeX{1024};
    std::atomic<u32> screenSizeY{512};

    void clear()
    {
        memset((void*)pixels,0,X*Y*4);
    }

    GLuint textures[2] = {};
    GLuint shaderProgram{};
    GLFWwindow* window{};

    void render()
    {
        glUniform1f(glGetUniformLocation(shaderProgram,"screenSizeX"), float(screenSizeX));
        glUniform1f(glGetUniformLocation(shaderProgram,"screenSizeY"), float(screenSizeY));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures[0]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, X, Y, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glfwPollEvents();
        glfwSwapBuffers(window);
    }
    void remake_buffers()
    {
        shaderProgram = createShaderProgram(vertexSourceMain, fragmentSourceMain);
        glDeleteTextures(2, textures);
        glGenTextures(2, textures);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures[0]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, X, Y, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glUseProgram(shaderProgram);
        glUniform1i(glGetUniformLocation(shaderProgram, "TextureSampler"), 0);
    }

    void SCREEN_start()
    {
        const char*const taglines[] =
        {
            "Granite - simply the best way to emulate a PC",
            "Granite - simply the best way to emulate a light pen",
            "Granite - simply the best way to emulate CGA",
            "Granite - simply the best way to emulate a CRT",
            "Granite - simply the best way to emulate AdLib",
            "Granite - simply the best way to emulate an XT keyboard",
            "Granite - simply the best way to emulate a RTC",
        };
        srand(time(nullptr));
        auto result = glfwInit();
        cout << "GLFWINIT result: " << result << endl;
        window = glfwCreateWindow(SCREEN_X, SCREEN_Y, taglines[rand()%7], nullptr, nullptr);
        glfwMakeContextCurrent(window);
        gladLoadGL((GLADloadfunc)glfwGetProcAddress);

        remake_buffers();
    }
    ~SCREEN()
    {
        glDeleteTextures(2, textures);
        glDeleteProgram(shaderProgram);
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    char infoLog[2048] = {};
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

    GLuint createShaderProgram(const char* vertexSource, const char* fragmentSource)
    {
        char processed_fragment_source[4096] = {};
        sprintf(processed_fragment_source, fragmentSource, SCREEN_X, SCREEN_Y, float(X), float(Y));

        GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
        GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, processed_fragment_source);

        GLuint prog = glCreateProgram();
        glAttachShader(prog, vertexShader);
        glAttachShader(prog, fragmentShader);
        glLinkProgram(prog);

        GLint success;
        glGetProgramiv(prog, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(prog, 2048, nullptr, infoLog);
            std::cerr << "Shader linking failed: " << infoLog << std::endl;
            std::abort();
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        return prog;
    }
} screen;
