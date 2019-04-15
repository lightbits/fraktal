// Developed by Simen Haugo.
// See LICENSE.txt for copyright and licensing details (standard MIT License).

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

#define IMGUI_IMPL_OPENGL_LOADER_GL3W
#include <imgui.cpp>
#include <imgui_draw.cpp>
#include <imgui_demo.cpp>
#include <imgui_widgets.cpp>
#include <imgui_impl_glfw.cpp>
#include <imgui_impl_opengl3.cpp>
#include <open_sans_regular.h>
#include "fraktal.h"

void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

int main(int argc, char **argv)
{
    fraktal_scene_def_t def = {0};
    def.render_shader_path = "./data/render/publication.glsl";
    def.model_shader_path = "./data/model/vase.glsl";
    def.compose_shader_path = "./data/compose/mean_and_gamma_correct.glsl";

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
    glfwSwapInterval(0);

    if (gl3wInit() != 0)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return 1;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(def.glsl_version);
    ImGui::GetStyle().WindowBorderSize = 0.0f;

    ImVector<ImWchar> glyph_ranges; // this must persist until call to GetTexData
    {
        const char *data = (const char*)open_sans_regular_compressed_data;
        const unsigned int size = open_sans_regular_compressed_size;
        float height = 16.0f;

        ImGuiIO &io = ImGui::GetIO();

        io.Fonts->AddFontFromMemoryCompressedTTF(data, size, height);

        // add math symbols with a different font size
        ImFontConfig config;
        config.MergeMode = true;
        ImFontGlyphRangesBuilder builder;
        builder.AddText("\xce\xb8\xcf\x86"); // theta, phi
        builder.BuildRanges(&glyph_ranges);
        io.Fonts->AddFontFromMemoryCompressedTTF(data, size, 18.0f, &config, glyph_ranges.Data);
    }

    fraktal_scene_t scene = {0};
    if (!fraktal_load(scene, def, 0))
        return 1;

    while (!glfwWindowShouldClose(window))
    {
        static int settle_frames = 0;
        if (settle_frames == 0)
        {
            glfwWaitEvents();
            settle_frames = 10;
        }
        else
        {
            glfwPollEvents();
            settle_frames--;
        }

        const double max_redraw_rate = 60.0;
        const double min_redraw_time = 1.0/max_redraw_rate;
        static double time_to_next_redraw = 0.0;
        static double t_prev = glfwGetTime();
        double t_curr = glfwGetTime();
        double t_delta = t_curr - t_prev;

        time_to_next_redraw -= t_delta;
        bool should_redraw = false;
        if (time_to_next_redraw < 0.0)
        {
            if (t_delta > min_redraw_time) // application is running slow
                time_to_next_redraw += min_redraw_time;
            else
                time_to_next_redraw += min_redraw_time - t_delta;
            should_redraw = true;
        }
        t_prev = t_curr;

        if (should_redraw)
        {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

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

            glfwMakeContextCurrent(window);
            int window_fb_width, window_fb_height;
            glfwGetFramebufferSize(window, &window_fb_width, &window_fb_height);
            glViewport(0, 0, window_fb_width, window_fb_height);
            glClearColor(0.14f, 0.14f, 0.14f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            fraktal_present(scene);

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(window);
        }
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
