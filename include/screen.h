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
        if (globalsettings.graphics == GlobalSettings::VGA) {
            const auto& buffer = mac.p.vga.get_pixel_buffer();
            int width = mac.p.vga.get_screen_width();
            int height = mac.p.vga.get_screen_height();

            if (!buffer.empty() && width > 0 && height > 0) {
                glUniform1f(glGetUniformLocation(shaderProgram,"screenSizeX"), float(width));
                glUniform1f(glGetUniformLocation(shaderProgram,"screenSizeY"), float(height));
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, textures[0]);

                GLint texWidth = 0, texHeight = 0;
                // It's good practice to bind the texture before querying its parameters
                glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &texWidth);
                glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &texHeight);

                if (texWidth != width || texHeight != height) {
                    // Texture dimensions or initial setup, use glTexImage2D
                    // VGA buffer is u32 ARGB (effectively 0xAARRGGBB on little endian like x86 for A=FF)
                    // OpenGL's GL_BGRA and GL_UNSIGNED_INT_8_8_8_8_REV expects BGRA order in memory for u32.
                    // If VGA's u32 is stored as BB GG RR AA (which is common if thinking ARGB bytes),
                    // then GL_BGRA with GL_UNSIGNED_INT_8_8_8_8_REV is correct.
                    // Assuming VGA pixel_buffer stores 0xAARRGGBB -> memory order is BB GG RR AA (little endian)
                    // So GL_BGRA (bytes B,G,R,A) with GL_UNSIGNED_INT_8_8_8_8_REV (reads u32, reverses byte order for GL's interpretation) is correct.
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, buffer.data());
                } else {
                    // Dimensions match, use glTexSubImage2D for performance
                    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, buffer.data());
                }
            }
        } else { // CGA or HEGA (or other non-VGA modes)
            // Assuming 'pixels' is u32 RGBA for this path, or needs conversion if different
            // The existing glTexSubImage2D uses GL_RGBA, GL_UNSIGNED_BYTE - this implies 'pixels' is an array of u8.
            // If 'pixels' is u32 and meant to be RGBA (e.g. 0xRRGGBBAA), then GL_RGBA with GL_UNSIGNED_INT_8_8_8_8 is more direct.
            // Given it's `pixels[1024*1025]`, and GL_UNSIGNED_BYTE, it's likely a byte array.
            // This part needs to be consistent with how CGA/HEGA populate `screen.pixels`.
            // For now, keeping original logic for CGA/HEGA:
            glUniform1f(glGetUniformLocation(shaderProgram,"screenSizeX"), float(screenSizeX));
            glUniform1f(glGetUniformLocation(shaderProgram,"screenSizeY"), float(screenSizeY));
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textures[0]);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, X, Y, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        }

        glDrawArrays(GL_TRIANGLES, 0, 3);
        glfwPollEvents();
        glfwSwapBuffers(window);
    }
    void remake_buffers()
    {
        shaderProgram = createShaderProgram(vertexSourceMain, fragmentSourceMain, X, Y);
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

} screen;
