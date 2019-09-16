/*
    Stand-alone GUI application for fraktal.
    Developed by Simen Haugo.
    See LICENSE.txt for copyright and licensing details (standard MIT License).
*/
#define FRAKTAL_GUI
#include "fraktal.cpp"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

#define IMGUI_IMPL_OPENGL_LOADER_GL3W
#include <imgui.cpp>
#include <imgui_draw.cpp>
#include <imgui_demo.cpp>
#include <imgui_widgets.cpp>
#include <imgui_impl_glfw.cpp>
#include <imgui_impl_opengl3.cpp>

#include <stdio.h>
#include <stdlib.h> // malloc, free
#include <string.h>
#include <file.h>
#include "fraktal.h"
#include "fraktal_parse.h"

#include <open_sans_semi_bold.h>

enum { MAX_WIDGETS = 128 };
enum { NUM_PRESETS = 10 };
struct Widget;
struct guiKey
{
    bool pressed;
    bool released;
    bool down;
};
struct guiKeys
{
    guiKey Space,Enter;
    guiKey Ctrl,Alt,Shift;
    guiKey Left,Right,Up,Down;
    guiKey W,A,S,D,P;
    guiKey PrintScreen;
    guiKey Num[10];
};
typedef int guiPreviewMode;
enum guiPreviewMode_ {
    guiPreviewMode_Color=0,
    guiPreviewMode_Thickness,
    guiPreviewMode_Normals,
    guiPreviewMode_Depth,
    guiPreviewMode_GBuffer,
};
struct guiPreset
{
    Widget *widgets[MAX_WIDGETS];
    int num_widgets;
};
struct guiSettings
{
    int width,height,x,y;
    float ui_scale;
};
struct guiPaths
{
    const char *model;
    const char *color;
    const char *geometry;
    const char *compose;
};
struct guiState
{
    guiPaths new_paths;
    int2 new_resolution;
    guiPreviewMode new_mode;

    const char *glsl_version;
    guiPaths paths;

    fArray *render_buffer;
    fArray *compose_buffer;
    fKernel *render_kernel;
    fKernel *compose_kernel;
    bool render_kernel_is_new;
    bool compose_kernel_is_new;
    int samples;
    int max_samples;
    bool should_clear;
    bool should_exit;
    bool initialized;
    bool auto_render;

    int2 resolution;
    guiKeys keys;
    guiPreviewMode mode;
    guiSettings settings;
    guiPreset *preset;
    guiPreset *next_preset;
    guiPreset presets[NUM_PRESETS];

    bool got_error;
};

#include "imgui_extensions.h"
#include "widgets/Widget.h"
#include "widgets/Sun.h"
#include "widgets/Camera.h"
#include "widgets/Ground.h"
#include "widgets/Material.h"
#include "widgets/Geometry.h"

static void save_screenshot(const char *filename, fArray *f)
{
    assert(filename);
    assert(f);
    assert(fraktal_array_format(f) == FRAKTAL_UINT8);
    int w,h; fraktal_array_size(f, &w,&h);
    int n = fraktal_array_channels(f);
    assert(w > 0 && h > 0 && n > 0);
    unsigned char *pixels = (unsigned char*)malloc(w*h*n);
    fraktal_to_cpu(pixels, f);
    stbi_write_png(filename, w, h, n, pixels, w*n);
    free(pixels);
}

static fKernel *load_render_shader(const char *model_path, const char *render_path)
{
    fLinkState *link = fraktal_create_link();

    static char *hg_sdf = read_file("libf/hg_sdf.f");
    if (!hg_sdf)
        log_err("Failed to load hg_sdf: file is corrupt or not in the expected directory (libf/hg_sdf.f)\n");

    if (hg_sdf)
    {
        char *model = read_file(model_path);
        if (!model)
        {
            log_err("Failed to load render kernel: error compiling model.\n");
            fraktal_destroy_link(link);
            return NULL;
        }
        static const char *line_0 = "#line 0\n";
        char *concat = (char*)malloc(strlen(hg_sdf) + strlen(line_0) + strlen(model) + 1);
        fraktal_assert(concat);
        strcpy(concat, hg_sdf);
        strcat(concat, line_0);
        strcat(concat, model);
        free(model);
        if (!fraktal_add_link_data(link, concat, strlen(concat), model_path))
        {
            log_err("Failed to load render kernel: error compiling model.\n");
            fraktal_destroy_link(link);
            free(concat);
            return NULL;
        }
        free(concat);
    }
    else
    {
        if (!fraktal_add_link_file(link, model_path))
        {
            log_err("Failed to load render kernel: error compiling model.\n");
            fraktal_destroy_link(link);
            return NULL;
        }
    }

    if (!fraktal_add_link_file(link, render_path))
    {
        log_err("Failed to load render kernel: error compiling renderer.\n");
        fraktal_destroy_link(link);
        return NULL;
    }

    fKernel *kernel = fraktal_link_kernel(link);
    fraktal_destroy_link(link);
    return kernel;
}

static bool load_gui(guiState &g)
{
    fKernel *render = NULL;
    if (g.new_mode == guiPreviewMode_Color)
        render = load_render_shader(g.new_paths.model, g.new_paths.color);
    else
        render = load_render_shader(g.new_paths.model, g.new_paths.geometry);

    if (!render)
    {
        log_err("Failed to load scene: error compiling render kernel.\n");
        return false;
    }

    fKernel *compose = fraktal_load_kernel(g.new_paths.compose);
    if (!compose)
    {
        log_err("Failed to load scene: error compiling compose kernel.\n");
        fraktal_destroy_kernel(render);
        return false;
    }

    // Refetch uniform offsets
    for (int preset = 0; preset < NUM_PRESETS; preset++)
    for (int widget = 0; widget < g.presets[preset].num_widgets; widget++)
    {
        assert(g.presets[preset].widgets[widget]);
        g.presets[preset].widgets[widget]->get_param_offsets(render);
    }

    // Destroy old state and update to newly loaded state
    fraktal_destroy_kernel(g.render_kernel);
    fraktal_destroy_kernel(g.compose_kernel);
    g.paths = g.new_paths;
    g.mode = g.new_mode;
    g.render_kernel = render;
    g.compose_kernel = compose;
    g.render_kernel_is_new = true;
    g.compose_kernel_is_new = true;
    g.should_clear = true;
    g.initialized = true;

    return true;
}

#define fetch_uniform(kernel, name) static int loc_##name; if (scene.kernel##_is_new) loc_##name = fraktal_get_param_offset(scene.kernel, #name);

static void render_color(guiState &scene)
{
    if (!scene.render_kernel || !scene.compose_kernel)
        return;
    assert(scene.render_kernel);
    assert(scene.compose_kernel);
    assert(fraktal_is_valid_array(scene.render_buffer));
    assert(fraktal_is_valid_array(scene.compose_buffer));

    // accumulation pass
    fraktal_use_kernel(scene.render_kernel);
    {
        fetch_uniform(render_kernel, iResolution);
        fetch_uniform(render_kernel, iSamples);
        scene.render_kernel_is_new = false;

        fArray *out = scene.render_buffer;
        if (scene.should_clear)
        {
            fraktal_zero_array(out);
            scene.samples = 0;
            scene.should_clear = false;
        }

        int width,height;
        fraktal_array_size(out, &width, &height);
        fraktal_param_2f(loc_iResolution, (float)width, (float)height);
        fraktal_param_1i(loc_iSamples, scene.samples);

        assert(scene.preset);
        for (int i = 0; i < scene.preset->num_widgets; i++)
        {
            if (scene.preset->widgets[i]->is_active())
                scene.preset->widgets[i]->set_params(scene);
        }

        fraktal_run_kernel(out);
        scene.samples++;
    }

    // compose pass
    fraktal_use_kernel(scene.compose_kernel);
    {
        fetch_uniform(compose_kernel, iResolution);
        fetch_uniform(compose_kernel, iChannel0);
        fetch_uniform(compose_kernel, iSamples);
        scene.compose_kernel_is_new = false;

        fArray *out = scene.compose_buffer;
        fArray *in = scene.render_buffer;
        int width,height;
        fraktal_array_size(out, &width, &height);
        fraktal_param_2f(loc_iResolution, (float)width, (float)height);
        fraktal_param_1i(loc_iSamples, scene.samples);
        fraktal_param_array(loc_iChannel0, in);

        fraktal_zero_array(out);
        fraktal_run_kernel(out);
    }

    fraktal_use_kernel(NULL);
}

static void render_geometry(guiState &scene)
{
    if (!scene.render_kernel)
        return;
    assert(scene.render_kernel);
    assert(fraktal_is_valid_array(scene.compose_buffer));

    // accumulation pass
    fraktal_use_kernel(scene.render_kernel);
    {
        fetch_uniform(render_kernel, iResolution);
        fetch_uniform(render_kernel, iDrawMode);
        scene.render_kernel_is_new = false;

        fArray *out = scene.compose_buffer;

        int width,height;
        fraktal_array_size(out, &width, &height);
        fraktal_param_2f(loc_iResolution, (float)width, (float)height);
        if      (scene.mode == guiPreviewMode_Normals) glUniform1i(loc_iDrawMode, 0);
        else if (scene.mode == guiPreviewMode_Depth) glUniform1i(loc_iDrawMode, 1);
        else if (scene.mode == guiPreviewMode_Thickness) glUniform1i(loc_iDrawMode, 2);
        else if (scene.mode == guiPreviewMode_GBuffer) glUniform1i(loc_iDrawMode, 3);
        else assert(false);

        assert(scene.preset);
        for (int i = 0; i < scene.preset->num_widgets; i++)
        {
            if (scene.preset->widgets[i]->is_active())
                scene.preset->widgets[i]->set_params(scene);
        }

        fraktal_zero_array(out);
        fraktal_run_kernel(out);
        scene.samples = 0;
        scene.should_clear = false;
    }
    fraktal_use_kernel(NULL);
}

static bool open_file_dialog(bool should_open, const char *label, char *buffer, size_t sizeof_buffer)
{
    if (should_open)
    {
        ImGui::OpenPopup(label);
        ImGui::CaptureKeyboardFromApp(true);
    }

    bool result = false;
    if (ImGui::BeginPopupModal(label, NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (ImGui::IsWindowAppearing())
            ImGui::SetKeyboardFocusHere();
        ImGui::InputText("Filename", buffer, sizeof_buffer);
        bool enter_key = ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter));
        if (ImGui::Button("OK", ImVec2(120, 0)) || enter_key)
        {
            result = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }
    return result;
}

static void allocate_or_resize_buffers(guiState &g)
{
    if (g.new_resolution.x <= 0)   g.new_resolution.x = 200;
    if (g.new_resolution.y <= 0)   g.new_resolution.y = 200;
    if (g.new_resolution.x > 2048) g.new_resolution.x = 2048;
    if (g.new_resolution.y > 2048) g.new_resolution.y = 2048;

    bool resolution_changed =
        g.resolution.x != g.new_resolution.x ||
        g.resolution.y != g.new_resolution.y;

    bool has_buffers =
        g.render_buffer != NULL &&
        g.compose_buffer != NULL;

    if (!has_buffers || resolution_changed)
    {
        fraktal_destroy_array(g.render_buffer);
        fraktal_destroy_array(g.compose_buffer);

        g.resolution.x = g.new_resolution.x;
        g.resolution.y = g.new_resolution.y;
        g.render_buffer =  fraktal_create_array(NULL, g.resolution.x, g.resolution.y, 4, FRAKTAL_FLOAT, FRAKTAL_READ_WRITE);
        g.compose_buffer = fraktal_create_array(NULL, g.resolution.x, g.resolution.y, 4, FRAKTAL_UINT8, FRAKTAL_READ_WRITE);
        g.should_clear = true;
    }
}

static void update_and_render_gui(guiState &scene)
{
    bool reload_key = scene.keys.Alt.down && scene.keys.Enter.pressed;
    static bool reload_request = true;

    if (scene.new_mode != scene.mode)
        reload_request = true;

    if (reload_key || (reload_request && !scene.got_error))
    {
        reload_request = false;
        log_clear();
        if (!load_gui(scene))
            scene.got_error = true;
        else
            scene.got_error = false;
    }

    for (int i = 0; i <= 9; i++)
    {
        if (scene.keys.Num[i].pressed)
            scene.next_preset = &scene.presets[i];
    }

    if (!scene.preset)
        scene.next_preset = &scene.presets[1];

    if (scene.preset != scene.next_preset)
    {
        scene.preset = scene.next_preset;
        scene.should_clear = true;
    }

    allocate_or_resize_buffers(scene);

    if (scene.mode == guiPreviewMode_Color)
    {
        if (!scene.keys.Alt.down && scene.keys.Enter.pressed)
            scene.auto_render = !scene.auto_render;
        if (scene.auto_render && scene.samples < scene.max_samples)
            render_color(scene);
        else if (scene.should_clear)
            render_color(scene);
    }
    else
    {
        if (scene.should_clear)
            render_geometry(scene);
    }

    if (scene.keys.P.pressed)
    {
        const char *name = "screenshot.png";
        if (scene.mode == guiPreviewMode_Color) name = "color.png";
        else if (scene.mode == guiPreviewMode_Thickness) name = "thickness.png";
        else if (scene.mode == guiPreviewMode_Normals) name = "normals.png";
        else if (scene.mode == guiPreviewMode_Depth) name = "depth.png";
        else if (scene.mode == guiPreviewMode_GBuffer) name = "gbuffer.png";
        save_screenshot(name, scene.compose_buffer);
    }

    float pad = 4.0f;

    int pushed_style_var = 0;
    int pushed_style_col = 0;

    pushed_style_var++; ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    pushed_style_var++; ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    pushed_style_var++; ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 4.0f);
    pushed_style_var++; ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, 0.0f);
    pushed_style_var++; ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 12.0f);

    pushed_style_col++; ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.825f));
    pushed_style_col++; ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 0.325f));
    pushed_style_col++; ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(1.0f, 1.0f, 1.0f, 0.078f));
    pushed_style_col++; ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(1.0f, 1.0f, 1.0f, 0.325f));
    pushed_style_col++; ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 0.325f));
    pushed_style_col++; ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.14f, 0.14f, 0.14f, 1.0f));
    pushed_style_col++; ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(0.3f, 0.38f, 0.51f, 1.0f));
    pushed_style_col++; ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.3f, 0.38f, 0.51f, 1.0f));
    pushed_style_col++; ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.3f, 0.38f, 0.51f, 1.0f));
    pushed_style_col++; ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(1.0f, 1.0f, 1.0f, 0.325f));
    pushed_style_col++; ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.0f,0.0f,0.0f,0.0f));
    pushed_style_col++; ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(1.0f,1.0f,1.0f,0.25f));
    pushed_style_col++; ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(1.0f,1.0f,1.0f,0.4f));
    pushed_style_col++; ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, ImVec4(1.0f,1.0f,1.0f,0.55f));
    pushed_style_col++; ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.22f, 0.22f, 0.22f, 1.0f));

    // main menu bar
    float main_menu_bar_height = 0.0f;
    bool open_screenshot_popup = false;
    bool open_model_popup = false;
    bool open_color_popup = false;
    {
        ImGui::BeginMainMenuBar();
        {
            main_menu_bar_height = ImGui::GetWindowHeight();
            // ImGui::Text("\xce\xb8"); // placeholder for Fraktal icon
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Load model##MainMenu"))        open_model_popup = true;
                if (ImGui::MenuItem("Load color shader##MainMenu")) open_color_popup = true;
                if (ImGui::MenuItem("Save as PNG##MainMenu", "P"))  open_screenshot_popup = true;
                ImGui::Separator();
                if (ImGui::MenuItem("Exit##MainMenu"))
                    scene.should_exit = true;
                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("Help"))
                ImGui::OpenPopup("Help##Popup");

            if (ImGui::BeginPopupModal("Help##Popup", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Separator();
                ImGui::Text("Keys");
                ImGui::Separator();
                ImGui::Columns(2);
                ImGui::BulletText("P");                  ImGui::NextColumn(); ImGui::Text("Take screenshot"); ImGui::NextColumn();
                ImGui::BulletText("Ctrl,Space,W,A,S,D"); ImGui::NextColumn(); ImGui::Text("Translate camera"); ImGui::NextColumn();
                ImGui::BulletText("Arrow keys");         ImGui::NextColumn(); ImGui::Text("Rotate camera"); ImGui::NextColumn();
                ImGui::BulletText("Enter");              ImGui::NextColumn(); ImGui::Text("Auto-render"); ImGui::NextColumn();
                ImGui::BulletText("Alt+Enter");          ImGui::NextColumn(); ImGui::Text("Reload model"); ImGui::NextColumn();
                ImGui::Columns(1);

                ImGui::Separator();
                ImGui::Text("Fraktal is built with the help of the following libraries:");
                ImGui::Separator();
                ImGui::BulletText("Dear ImGui");
                ImGui::BulletText("GLFW");
                ImGui::BulletText("GL3W");
                ImGui::BulletText("stb_image and stb_image_write");
                if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
                ImGui::EndPopup();
            }
        }
        ImGui::EndMainMenuBar();
    }

    // side panel
    float side_panel_width = 0.0f;
    float side_panel_height = 0.0f;
    {
        ImGuiIO &io = ImGui::GetIO();
        float avail_y = io.DisplaySize.y - main_menu_bar_height - pad;
        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse;
        ImGui::SetNextWindowSize(ImVec2(0.3f*io.DisplaySize.x, avail_y), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, avail_y), ImVec2(io.DisplaySize.x, avail_y));
        ImGui::SetNextWindowPos(ImVec2(pad, main_menu_bar_height));

        ImGui::Begin("Scene parameters", NULL, flags);
        side_panel_width = ImGui::GetWindowWidth();
        side_panel_height = ImGui::GetWindowHeight();

        // buttons for swapping presets
        for (int i = 1; i <= 10; i++)
        {
            static const char *label[] = {
                "0##preset buttons",
                "1##preset buttons",
                "2##preset buttons",
                "3##preset buttons",
                "4##preset buttons",
                "5##preset buttons",
                "6##preset buttons",
                "7##preset buttons",
                "8##preset buttons",
                "9##preset buttons"
            };
            int j = i % 10;
            if (scene.preset == &scene.presets[j])
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 0.078f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.325f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.325f));
            }

            if (ImGui::Button(label[j]))
                scene.next_preset = &scene.presets[j];

            ImGui::PopStyleColor();
            ImGui::PopStyleColor();

            if (i < 10)
                ImGui::SameLine();
        }

        assert(scene.preset);
        for (int i = 0; i < scene.preset->num_widgets; i++)
        {
            if (scene.preset->widgets[i]->is_active())
                scene.should_clear |= scene.preset->widgets[i]->update(scene);
        }

        ImGui::End();
    }

    // tabs in main menu bar
    {
        ImGui::BeginMainMenuBar();
        ImGui::SetCursorPosX(side_panel_width + pad + pad + 8.0f);
        if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_None))
        {
            if (ImGui::BeginTabItem("Color"))     { scene.new_mode = guiPreviewMode_Color; ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Thickness")) { scene.new_mode = guiPreviewMode_Thickness; ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Normals"))   { scene.new_mode = guiPreviewMode_Normals; ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Depth"))     { scene.new_mode = guiPreviewMode_Depth; ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("GBuffer"))   { scene.new_mode = guiPreviewMode_GBuffer; ImGui::EndTabItem(); }
            ImGui::EndTabBar();
        }
        ImGui::EndMainMenuBar();
    }

    // error list panel
    float error_list_height = 0.0f;
    if (scene.got_error)
    {
        ImGuiIO &io = ImGui::GetIO();
        float width = io.DisplaySize.x - (side_panel_width + pad + pad) - pad;
        float x = side_panel_width + pad + pad;
        float y = io.DisplaySize.y - 200.0f - pad;
        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse;

        ImGui::SetNextWindowSize(ImVec2(width, 200.0f));
        ImGui::SetNextWindowPos(ImVec2(x, y));
        ImGui::Begin("Error list", NULL, flags);
        error_list_height = ImGui::GetWindowHeight() + pad;
        ImGui::TextWrapped(log_get_buffer());
        ImGui::End();
    }

    // main preview panel
    {
        ImGuiIO &io = ImGui::GetIO();
        float height = side_panel_height - error_list_height;
        float width = io.DisplaySize.x - (side_panel_width + pad + pad) - pad;

        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_MenuBar;
        ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.3f, 0.38f, 0.51f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.69f, 0.69f, 0.69f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f,0.0f));
        ImGui::SetNextWindowSize(ImVec2(width, height));
        ImGui::SetNextWindowPos(ImVec2(side_panel_width + pad + pad, main_menu_bar_height));
        ImGui::Begin("Preview", NULL, flags);

        {
            const int display_mode_1x = 0;
            const int display_mode_2x = 1;
            const int display_mode_fit = 2;
            static int display_mode = display_mode_1x;

            ImGui::BeginMenuBar();
            {
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f,4.0f));

                ImGui::PushItemWidth(128.0f);
                static int2 resolution = scene.new_resolution;
                if (ImGui::InputInt2("##resolution", &resolution.x, ImGuiInputTextFlags_EnterReturnsTrue))
                    scene.new_resolution = resolution;
                ImGui::PopItemWidth();

                // ImGui::Text("%d x %d", scene.resolution.x, scene.resolution.y);
                ImGui::Separator();
                const char *scale_label = "1x###Scale";
                if      (display_mode == display_mode_1x)  scale_label = "Scale 1x###Scale";
                else if (display_mode == display_mode_2x)  scale_label = "Scale 2x###Scale";
                else if (display_mode == display_mode_fit) scale_label = "Scale to fit###Scale";
                if (ImGui::BeginMenu(scale_label))
                {
                    if (ImGui::MenuItem("1x", NULL, display_mode==display_mode_1x)) { display_mode = display_mode_1x; }
                    if (ImGui::MenuItem("2x", NULL, display_mode==display_mode_2x)) { display_mode = display_mode_2x; }
                    if (ImGui::MenuItem("Scale to fit", NULL, display_mode==display_mode_fit)) { display_mode = display_mode_fit; }
                    ImGui::EndMenu();
                }
                ImGui::PopStyleVar();
                if (scene.mode == guiPreviewMode_Color)
                {
                    ImGui::Separator();
                    ImGui::Text("Samples: %d / ", scene.samples);
                    ImGui::PushItemWidth(64.0f);
                    if (ImGui::DragInt("##max_samples", &scene.max_samples, 1.0f, 1, 2048))
                        scene.should_clear = true;
                    ImGui::PopItemWidth();
                }
            }
            ImGui::EndMenuBar();

            ImDrawList *draw = ImGui::GetWindowDrawList();
            {
                int width,height;
                fraktal_array_size(scene.compose_buffer, &width, &height);
                unsigned int texture = fraktal_get_gl_handle(scene.compose_buffer);

                ImVec2 image_size = ImVec2((float)width, (float)height);
                if (io.DisplayFramebufferScale.x > 0.0f &&
                    io.DisplayFramebufferScale.y > 0.0f)
                {
                    image_size.x /= io.DisplayFramebufferScale.x;
                    image_size.y /= io.DisplayFramebufferScale.y;
                }
                ImVec2 avail = ImGui::GetContentRegionAvail();

                ImVec2 top_left = ImGui::GetCursorScreenPos();
                ImVec2 pos0,pos1;
                if (display_mode == display_mode_1x)
                {
                    pos0.x = (float)(int)(top_left.x + avail.x*0.5f - image_size.x*0.5f);
                    pos0.y = (float)(int)(top_left.y + avail.y*0.5f - image_size.y*0.5f);
                    pos1.x = (float)(int)(top_left.x + avail.x*0.5f + image_size.x*0.5f);
                    pos1.y = (float)(int)(top_left.y + avail.y*0.5f + image_size.y*0.5f);
                }
                else if (display_mode == display_mode_2x)
                {
                    pos0.x = (float)(int)(top_left.x + avail.x*0.5f - image_size.x);
                    pos0.y = (float)(int)(top_left.y + avail.y*0.5f - image_size.y);
                    pos1.x = (float)(int)(top_left.x + avail.x*0.5f + image_size.x);
                    pos1.y = (float)(int)(top_left.y + avail.y*0.5f + image_size.y);
                }
                else if (avail.x < avail.y)
                {
                    float h = image_size.y*avail.x/image_size.x;
                    pos0.x = (float)(int)(top_left.x);
                    pos0.y = (float)(int)(top_left.y + avail.y*0.5f - h*0.5f);
                    pos1.x = (float)(int)(top_left.x + avail.x);
                    pos1.y = (float)(int)(top_left.y + avail.y*0.5f + h*0.5f);
                }
                else if (avail.y < avail.x)
                {
                    float w = image_size.x*avail.y/image_size.y;
                    pos0.x = (float)(int)(top_left.x + avail.x*0.5f - w*0.5f);
                    pos0.y = (float)(int)(top_left.y);
                    pos1.x = (float)(int)(top_left.x + avail.x*0.5f + w*0.5f);
                    pos1.y = (float)(int)(top_left.y + avail.y);
                }

                {
                    draw->PushClipRect(pos0, pos1, true);
                    float checker_size_x = 30.0f;
                    float checker_size_y = 30.0f;
                    int num_checkers_x = (int)((pos1.x - pos0.x) / checker_size_x + 1);
                    int num_checkers_y = (int)((pos1.y - pos0.y) / checker_size_y + 1);
                    for (int yi = 0; yi < num_checkers_y; yi++)
                    for (int xi = 0; xi < num_checkers_x; xi += 2)
                    {
                        float x = (yi % 2 == 0) ?
                            pos0.x + xi*checker_size_x :
                            pos0.x + (xi+1)*checker_size_x;
                        float y = pos0.y + yi*checker_size_y;
                        draw->AddRectFilled(ImVec2(x, y), ImVec2(x + checker_size_x, y + checker_size_y), IM_COL32(0,0,0,50));
                    }
                    draw->PopClipRect();
                }

                ImU32 tint = 0xFFFFFFFF;
                ImVec2 uv0 = ImVec2(0.0f,0.0f);
                ImVec2 uv1 = ImVec2(1.0f,1.0f);
                draw->AddImage((void*)(intptr_t)texture, pos0, pos1, uv0, uv1, tint);
            }
        }

        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
    }

    {
        static char path[1024];
        if (open_file_dialog(open_model_popup, "Open model##Popup", path, sizeof(path)))
        {
            scene.new_paths.model = path;
            reload_request = true;
        }
    }
    {
        static char path[1024];
        if (open_file_dialog(open_color_popup, "Load color shader##Popup", path, sizeof(path)))
        {
            scene.new_paths.color = path;
            reload_request = true;
        }
    }
    {
        static char path[1024];
        if (open_file_dialog(open_screenshot_popup, "Save as PNG##Popup", path, sizeof(path)))
            save_screenshot(path, scene.compose_buffer);
    }

    ImGui::PopStyleVar(pushed_style_var);
    ImGui::PopStyleColor(pushed_style_col);
}

static guiState g_scene;
static int g_window_pos_x = -1;
static int g_window_pos_y = -1;

static void glfw_window_pos_callback(GLFWwindow *window, int x, int y)
{
    g_window_pos_x = x;
    g_window_pos_y = y;
}

enum { NUM_GLFW_KEYS = 512 };
static struct glfw_key_t
{
    bool was_pressed;
    bool was_released;
    bool is_down;
} glfw_keys[NUM_GLFW_KEYS];

static void mark_key_events_as_processed()
{
    for (int key = 0; key < NUM_GLFW_KEYS; key++)
    {
        glfw_keys[key].was_pressed = false;
        glfw_keys[key].was_released = false;
    }
}

static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key >= 0 && key < NUM_GLFW_KEYS)
    {
        bool was_down = glfw_keys[key].is_down;
        if (action == GLFW_PRESS && !was_down) {
            glfw_keys[key].was_pressed = true;
            glfw_keys[key].is_down = true;
        }
        if (action == GLFW_RELEASE && was_down) {
            glfw_keys[key].was_released = true;
            glfw_keys[key].is_down = false;
        }
        if (action == GLFW_REPEAT) {
            glfw_keys[key].was_pressed = true;
            glfw_keys[key].is_down = true;
        }
    }
}

static void write_settings_to_disk(const char *ini_filename, guiState s)
{
    FILE *f = fopen(ini_filename, "wt");
    if (!f)
        return;

    fprintf(f, "fraktal = \n(\n");
    fprintf(f, "\twindow_size=(%d, %d),\n", s.settings.width, s.settings.height);
    fprintf(f, "\twindow_position=(%d, %d),\n", s.settings.x, s.settings.y);
    fprintf(f, "\tui_scale=%g\n", s.settings.ui_scale);
    fprintf(f, ")\n\n");

    size_t imgui_ini_size = 0;
    const char *imgui_ini_data = ImGui::SaveIniSettingsToMemory(&imgui_ini_size);
    fwrite(imgui_ini_data, sizeof(char), imgui_ini_size, f);

    fclose(f);
}

static void read_settings_from_disk(const char *ini_filename, guiState &g)
{
    char *f = read_file(ini_filename);
    if (!f)
        return;

    parse_error_start = f;
    parse_error_name = "settings";

    const char *cc = f;
    const char **c = &cc;
    parse_blank(c);
    if (!parse_match(c, "fraktal")) return;
    parse_blank(c);
    if (!parse_char(c, '=')) return;
    parse_blank(c);
    if (!parse_begin_list(c)) return;
    while (parse_next_in_list(c))
    {
        int2 window_size;
        int2 window_position;
        float ui_scale;
        if (parse_argument_int2(c, "window_size", &window_size)) { g.settings.width = window_size.x; g.settings.height = window_size.y; }
        else if (parse_argument_int2(c, "window_position", &window_position)) { g.settings.x = window_position.x; g.settings.y = window_position.y; }
        else if (parse_argument_float(c, "ui_scale", &ui_scale)) { g.settings.ui_scale = ui_scale; }
        else parse_list_unexpected();
    }
    parse_end_list(c);
    parse_blank(c);

    // the rest of the ini file is ImGui settings
    ImGui::LoadIniSettingsFromMemory(*c);

    free(f);
}

static void default_settings(guiState &g)
{
    g.settings.width = 800;
    g.settings.height = 600;
    g.settings.x = -1;
    g.settings.y = -1;
    g.settings.ui_scale = 1.0f;
    g.max_samples = 128;
}

static void sanitize_settings(guiState &g)
{
    if (g.settings.width < 1)
        g.settings.width = 800;
    if (g.settings.height < 1)
        g.settings.height = 600;
    if (g.settings.ui_scale < 0.5f || g.settings.ui_scale > 4.0f)
        g.settings.ui_scale = 1.0f;
}

int main(int argc, char **argv)
{
    const char *ini_filename = "fraktal.ini";
    g_scene.new_paths.model    = "examples/vase.f";
    g_scene.new_paths.color    = "libf/publication.f";
    g_scene.new_paths.geometry = "libf/geometry.f";
    g_scene.new_paths.compose  = "libf/compose.f";
    g_scene.new_resolution.x   = 320;
    g_scene.new_resolution.y   = 240;
    g_scene.new_mode           = guiPreviewMode_Color;

    fraktal_create_context();

    if (!fraktal_context)
    {
        log_err("The fraktal GUI requires you to create a context for fraktal (use fraktal_create_context).\n");
        return 1;
    }

    // set up ImGui
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui::GetStyle().WindowBorderSize = 0.0f;
    ImGui::GetIO().IniFilename = NULL; // override ImGui load/save ini behavior with our own

    // add default widgets
    for (int preset = 0; preset < NUM_PRESETS; preset++)
    {
        guiPreset &p = g_scene.presets[preset];
        p.num_widgets = 0;
        p.widgets[p.num_widgets++] = new Widget_Camera;
        p.widgets[p.num_widgets++] = new Widget_Sun;
        p.widgets[p.num_widgets++] = new Widget_Material;
        p.widgets[p.num_widgets++] = new Widget_Ground;
        p.widgets[p.num_widgets++] = new Widget_Geometry;
        for (int i = 0; i < p.num_widgets; i++)
            p.widgets[i]->default_values();
    }
    g_scene.next_preset = &g_scene.presets[1];

    default_settings(g_scene);
    read_settings_from_disk(ini_filename, g_scene);
    sanitize_settings(g_scene);

    // this must be called before any OpenGL operations
    fraktal_ensure_context();

    // create window
    guiSettings settings = g_scene.settings;
    if (settings.x >= 0 && settings.y >= 0)
        glfwSetWindowPos(fraktal_context, settings.x, settings.y);
    glfwSetWindowSize(fraktal_context, settings.width, settings.height);
    glfwShowWindow(fraktal_context);
    glfwSwapInterval(0);
    glfwSetKeyCallback(fraktal_context, glfw_key_callback);
    glfwSetWindowPosCallback(fraktal_context, glfw_window_pos_callback);

    // initialize OpenGL state
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    ImGui_ImplGlfw_InitForOpenGL(fraktal_context, true);
    ImGui_ImplOpenGL3_Init(fraktal_glsl_version);

    // load fonts
    {
        ImGuiIO &io = ImGui::GetIO();

        // add main font
        float font_size = 16.0f*settings.ui_scale;
        const char *data = (const char*)open_sans_semi_bold_compressed_data;
        const unsigned int sizeof_data = open_sans_semi_bold_compressed_size;
        io.Fonts->AddFontFromMemoryCompressedTTF(data, sizeof_data, font_size);


        // add math symbols (larger)
        float math_size = 18.0f*settings.ui_scale;
        ImFontConfig config;
        config.MergeMode = true;
        ImFontGlyphRangesBuilder builder;
        builder.AddText("\xce\xb8\xcf\x86\xe0\x04"); // theta, phi
        static ImVector<ImWchar> glyph_ranges; // this must persist until call to GetTexData
        builder.BuildRanges(&glyph_ranges);
        io.Fonts->AddFontFromMemoryCompressedTTF(data, sizeof_data, math_size, &config, glyph_ranges.Data);
    }

    // reminder for future: these must be called in order before the main loop below
    // fraktal_ensure_context();
    // glfwShowWindow(fraktal_context);

    while (!glfwWindowShouldClose(fraktal_context) && !g_scene.should_exit)
    {
        static int settle_frames = 5;
        if (g_scene.auto_render || settle_frames > 0)
        {
            glfwPollEvents();
        }
        else
        {
            glfwWaitEvents();
            settle_frames = 5;
        }

        // update settings
        guiSettings &settings = g_scene.settings;
        settings.x = g_window_pos_x;
        settings.y = g_window_pos_y;
        glfwGetWindowSize(fraktal_context, &settings.width, &settings.height);

        const double max_redraw_rate = 60.0;
        const double min_redraw_time = 1.0/max_redraw_rate;
        static double t_last_redraw = -min_redraw_time;
        double t_curr = glfwGetTime();
        double t_delta = t_curr - t_last_redraw;
        bool should_redraw = false;
        if (t_delta >= min_redraw_time)
        {
            t_last_redraw = t_curr;
            should_redraw = true;
        }

        if (should_redraw)
        {
            settle_frames--;
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            #define copy_key_event(struct_name, glfw_key) \
                g_scene.keys.struct_name.pressed = glfw_keys[glfw_key].was_pressed && !ImGui::GetIO().WantCaptureKeyboard; \
                g_scene.keys.struct_name.released = glfw_keys[glfw_key].was_released && !ImGui::GetIO().WantCaptureKeyboard; \
                g_scene.keys.struct_name.down = glfw_keys[glfw_key].is_down && !ImGui::GetIO().WantCaptureKeyboard;
            copy_key_event(Enter, GLFW_KEY_ENTER);
            copy_key_event(Space, GLFW_KEY_SPACE);
            copy_key_event(Ctrl, GLFW_KEY_LEFT_CONTROL);
            copy_key_event(Alt, GLFW_KEY_LEFT_ALT);
            copy_key_event(Shift, GLFW_KEY_LEFT_SHIFT);
            copy_key_event(Left, GLFW_KEY_LEFT);
            copy_key_event(Right, GLFW_KEY_RIGHT);
            copy_key_event(Up, GLFW_KEY_UP);
            copy_key_event(Down, GLFW_KEY_DOWN);
            copy_key_event(W, GLFW_KEY_W);
            copy_key_event(A, GLFW_KEY_A);
            copy_key_event(S, GLFW_KEY_S);
            copy_key_event(D, GLFW_KEY_D);
            copy_key_event(P, GLFW_KEY_P);
            copy_key_event(PrintScreen, GLFW_KEY_PRINT_SCREEN);
            for (int i = 0; i <= 9; i++)
            {
                g_scene.keys.Num[i].pressed = glfw_keys[GLFW_KEY_0 + i].was_pressed && !ImGui::GetIO().WantCaptureKeyboard;
                g_scene.keys.Num[i].released = glfw_keys[GLFW_KEY_0 + i].was_released && !ImGui::GetIO().WantCaptureKeyboard;
                g_scene.keys.Num[i].down = glfw_keys[GLFW_KEY_0 + i].is_down && !ImGui::GetIO().WantCaptureKeyboard;
            }
            mark_key_events_as_processed();

            glfwMakeContextCurrent(fraktal_context);
            int window_fb_width, window_fb_height;
            glfwGetFramebufferSize(fraktal_context, &window_fb_width, &window_fb_height);
            glViewport(0, 0, window_fb_width, window_fb_height);
            glClearColor(0.14f, 0.14f, 0.14f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            update_and_render_gui(g_scene);

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            if (ImGui::GetIO().WantSaveIniSettings)
            {
                write_settings_to_disk(ini_filename, g_scene);
                ImGui::GetIO().WantSaveIniSettings = false;
            }

            glfwSwapBuffers(fraktal_context);
        }
    }
    write_settings_to_disk(ini_filename, g_scene);

    return 0;
}
