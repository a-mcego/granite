#pragma once

#include "shader.h"
#include "palette.h"

struct SCREEN
{
    u32 pixels[912*264] = {};
    u16 X{912}, Y{262};

    GLuint textures[2] = {};
    GLuint shaderProgram{};
    GLFWwindow* window{};

    void render()
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures[0]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, X, Y, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    void remake_buffers()
    {
        shaderProgram = createShaderProgram(vertexSource, fragmentSource, X, Y);
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
        auto result = glfwInit();
        cout << "GLFWINIT result: " << result << endl;
        window = glfwCreateWindow(SCREEN_X, SCREEN_Y, "Granite", nullptr, nullptr);
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

} screen;
