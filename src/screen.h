#pragma once

#include "shader.h"
#include "palette.h"

struct SCREEN
{
    u8 pixels[640*400] = {};
    u16 X{640}, Y{200};
    GLuint textures[2] = {};
    GLuint shaderProgram{};
    GLFWwindow* window{};

    void render()
    {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, X, Y, GL_RED_INTEGER, GL_UNSIGNED_BYTE, pixels);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    void remake_buffers()
    {
        shaderProgram = createShaderProgram(vertexSource, fragmentSource, X, Y);
        glDeleteTextures(2, textures);
        glGenTextures(2, textures);
        glBindTexture(GL_TEXTURE_2D, textures[0]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8UI, X, Y, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glBindTexture(GL_TEXTURE_1D, textures[1]);
        glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA8, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, PALETTE);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glUseProgram(shaderProgram);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures[0]);
        glUniform1i(glGetUniformLocation(shaderProgram, "TextureSampler"), 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_1D, textures[1]);
        glUniform1i(glGetUniformLocation(shaderProgram, "PaletteSampler"), 1);

        glActiveTexture(GL_TEXTURE0);

    }

    void SCREEN_start()
    {
        auto result = glfwInit();
        cout << "GLFWINIT result: " << result << endl;
        window = glfwCreateWindow(1280, 720, "Granite", nullptr, nullptr);
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
