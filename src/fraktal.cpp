#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <GL/gl3w.h>
#include <GL/gl3w.c>
#include <GLFW/glfw3.h>
#include <string.h>

// Required to link with vc2010 GLFW
#if defined(_MSC_VER) && (_MSC_VER >= 1900)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

#if 0
#include "scene_parser.h"
#include "cli_parser.h"

int main(int argc, char **argv)
{
    if (!test_scene_parser())
        printf("terrible stuff happened: %s\n", log_get_buffer());
    assert(test_cli_parser());

    return 0;
}
#else
#define IMGUI_IMPL_OPENGL_LOADER_GL3W
#include <imgui.cpp>
#include <imgui_draw.cpp>
#include <imgui_demo.cpp>
#include <imgui_widgets.cpp>
#include <imgui_impl_glfw.cpp>
#include <imgui_impl_opengl3.cpp>
#include <open_sans_regular.h>

#include "fraktal.h"
#include "cli_parser.h"

void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

int main(int argc, char **argv)
{
    fraktal_scene_def_t def = {0};
    def.render_shader_path = "./data/default_render_gi.glsl";
    def.model_shader_path = "./data/default_model.glsl";
    def.compose_shader_path = "./data/default_compose.glsl";

    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return EXIT_FAILURE;

    #if __APPLE__
    def.glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
    #else
    def.glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    #endif

    GLFWwindow* window = glfwCreateWindow(600, 400, "fraktal", NULL, NULL);
    if (window == NULL)
        return EXIT_FAILURE;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (gl3wInit() != 0)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return 1;
    }

    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(def.glsl_version);
    ImGui::GetStyle().WindowBorderSize = 0.0f;

    {
        const char *data = (const char*)open_sans_regular_compressed_data;
        const unsigned int size = open_sans_regular_compressed_size;
        float height = 17.0f;
        ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(data, size, height);
    }

    fraktal_scene_t scene = {0};
    assert(fraktal_load(scene, def, 0));

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        glfwMakeContextCurrent(window);
        int window_fb_width, window_fb_height;
        glfwGetFramebufferSize(window, &window_fb_width, &window_fb_height);
        glViewport(0, 0, window_fb_width, window_fb_height);
        glClearColor(0.14f, 0.14f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        {
            #define handle_key(struct_name, key) { \
                bool down = glfwGetKey(window, key) == GLFW_PRESS; \
                scene.keys.struct_name.released = false; \
                scene.keys.struct_name.pressed = false; \
                if (scene.keys.struct_name.down && !down) \
                    scene.keys.struct_name.released = true; \
                if (!scene.keys.struct_name.down && down) \
                    scene.keys.struct_name.pressed = true; \
                scene.keys.struct_name.down = down; \
                }
            handle_key(Enter, GLFW_KEY_ENTER);
            handle_key(Space, GLFW_KEY_SPACE);
            handle_key(Ctrl, GLFW_KEY_LEFT_CONTROL);
            handle_key(Alt, GLFW_KEY_LEFT_ALT);
            handle_key(Shift, GLFW_KEY_LEFT_SHIFT);
            handle_key(Left, GLFW_KEY_LEFT);
            handle_key(Right, GLFW_KEY_RIGHT);
            handle_key(Up, GLFW_KEY_UP);
            handle_key(Down, GLFW_KEY_DOWN);
            handle_key(W, GLFW_KEY_W);
            handle_key(A, GLFW_KEY_A);
            handle_key(S, GLFW_KEY_S);
            handle_key(D, GLFW_KEY_D);
        }
        fraktal_present(scene);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
#endif
