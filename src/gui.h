// Developed by Simen Haugo.
// See LICENSE.txt for copyright and licensing details (standard MIT License).

#include "fraktal.h"
#include "fraktal_parse.h"
#include "gui_state.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>
#include <stdio.h>
#include <stdlib.h> // malloc, free
#include <string.h>
#include <GL/gl3w.h>
#include <file.h>

#include "gui_state.h"
#include "widgets/Widget.h"
#include "widgets/Sun.h"
#include "widgets/Camera.h"
#include "widgets/Floor.h"
#include "widgets/Material.h"
#include "widgets/Geometry.h"

void remove_directive_from_source(char *from, char *to)
{
    for (char *c = from; c < to; c++)
        *c = ' ';
}

bool scene_file_preprocessor(char *fs, guiSceneParams *params)
{
    params->num_widgets = 0;
    char *c = fs;
    while (*c)
    {
        parse_comment((const char **)&c);
        if (*c == '#')
        {
            char *mark = c;
            c++;
            const char **cc = (const char **)&c;
            if (parse_match(cc, "widget"))
            {
                if (params->num_widgets == MAX_WIDGETS) { log_err("Exceeded the maximum number of widgets.\n"); goto failure; }

                parse_alpha(cc);
                parse_blank(cc);
                if (!parse_begin_list(cc)) { log_err("Error parsing #widget directive: missing ( after '#sun'.\n"); goto failure; }

                // horrible macro mess
                #define PARSE_WIDGET(widget) \
                    if (parse_match(cc, #widget)) { \
                        parse_char(cc, ','); \
                        Widget_##widget *ww = new Widget_##widget(params, cc); \
                        if (!parse_end_list(cc)) { free(ww); log_err("Error parsing " #widget " #widget directive.\n"); goto failure; } \
                        params->widgets[params->num_widgets] = ww; \
                    } else

                PARSE_WIDGET(Sun)
                PARSE_WIDGET(Camera)
                PARSE_WIDGET(Floor)
                PARSE_WIDGET(Material)
                PARSE_WIDGET(Geometry)
                // ...
                // add new macros here!
                // ...
                { log_err("Error parsing #widget directive: unknown widget type.\n"); goto failure; }

                params->num_widgets++;
                remove_directive_from_source(mark, c);
            }
        }
        c++;
    }
    return true;

failure:
    for (int i = 0; i < params->num_widgets; i++)
        free(params->widgets[i]);
    return false;
}

void save_screenshot(const char *filename, fArray *f)
{
    assert(filename);
    assert(f);
    assert(fraktal_array_format(f) == FRAKTAL_UINT8);
    int w,h; fraktal_array_size(f, &w,&h);
    int n = fraktal_array_channels(f);
    assert(w > 0 && h > 0 && n > 0);
    unsigned char *pixels = (unsigned char*)malloc(w*h*n);
    fraktal_gpu_to_cpu(pixels, f);
    stbi_write_png(filename, w, h, n, pixels, w*n);
    free(pixels);
}

fKernel *load_render_shader(
    const char *model_kernel_path,
    const char *render_kernel_path,
    guiSceneParams *params)
{
    fLinkState *link = fraktal_create_link();

    if (!fraktal_add_link_file(link, model_kernel_path))
    {
        log_err("Failed to load render kernel: error compiling model.\n");
        fraktal_destroy_link(link);
        return NULL;
    }

    // render kernel
    {
        const char *path = render_kernel_path;
        char *data = read_file(path);
        if (!data)
        {
            log_err("Failed to load render kernel: could not read file '%s'.\n", path);
            fraktal_destroy_link(link);
            return NULL;
        }
        if (!scene_file_preprocessor(data, params))
        {
            log_err("Failed to load render kernel: could not parse file '%s'.\n", path);
            fraktal_destroy_link(link);
            free(data);
            return NULL;
        }
        if (!fraktal_add_link_data(link, data, 0, path))
        {
            log_err("Failed to load render kernel: error compiling '%s'.\n", path);
            fraktal_destroy_link(link);
            free(data);
            return NULL;
        }
        free(data);
    }

    fKernel *kernel = fraktal_link_kernel(link);
    fraktal_destroy_link(link);
    return kernel;
}

bool gui_load(guiState &scene, guiSceneDef def)
{
    assert(def.resolution_x > 0 && def.resolution_y > 0);
    guiSceneParams params = scene.params;

    params.resolution.x = def.resolution_x;
    params.resolution.y = def.resolution_y;

    fKernel *render = NULL;
    {
        if (scene.mode == guiPreviewMode_Color) render = load_render_shader(def.model_kernel_path, def.color_kernel_path, &params);
        else render = load_render_shader(def.model_kernel_path, def.geometry_kernel_path, &params);
    }

    if (!render)
    {
        log_err("Failed to load scene: error compiling render kernel.\n");
        return false;
    }

    for (int i = 0; i < params.num_widgets; i++)
        params.widgets[i]->get_param_offsets(render);

    fKernel *compose = fraktal_load_kernel(def.compose_kernel_path);
    if (!compose)
    {
        log_err("Failed to load scene: error compiling compose kernel.\n");
        fraktal_destroy_kernel(render);
        return false;
    }

    // Reallocate output buffers if the resolution changed or
    // if this is the first time we load the scene.
    bool resolution_changed =
        params.resolution.x != scene.params.resolution.x ||
        params.resolution.y != scene.params.resolution.y;
    if (!scene.initialized || resolution_changed)
    {
        int x = params.resolution.x;
        int y = params.resolution.y;
        assert(x > 0);
        assert(y > 0);

        fraktal_destroy_array(scene.render_buffer);
        fraktal_destroy_array(scene.compose_buffer);

        scene.params.resolution.x = x;
        scene.params.resolution.y = y;
        scene.render_buffer = fraktal_create_array(NULL, x, y, 4, FRAKTAL_FLOAT, FRAKTAL_READ_WRITE);
        scene.compose_buffer = fraktal_create_array(NULL, x, y, 4, FRAKTAL_UINT8, FRAKTAL_READ_WRITE);
        scene.should_clear = true;
    }

    // Destroy old state and update to newly loaded state
    for (int i = 0; i < scene.params.num_widgets; i++)
        free(scene.params.widgets[i]);
    fraktal_destroy_kernel(scene.render_kernel);
    fraktal_destroy_kernel(scene.compose_kernel);
    scene.render_kernel = render;
    scene.compose_kernel = compose;
    scene.render_kernel_is_new = true;
    scene.compose_kernel_is_new = true;
    scene.params = params;
    scene.should_clear = true;
    scene.def = def;
    scene.initialized = true;

    return true;
}

#define fetch_uniform(kernel, name) static int loc_##name; if (scene.kernel##_is_new) loc_##name = fraktal_get_param_offset(scene.kernel, #name);

void render_color(guiState &scene)
{
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
        glUniform2f(loc_iResolution, (float)width, (float)height);
        glUniform1i(loc_iSamples, scene.samples);

        for (int i = 0; i < scene.params.num_widgets; i++)
            scene.params.widgets[i]->set_params();

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
        glUniform2f(loc_iResolution, (float)width, (float)height);
        glUniform1i(loc_iSamples, scene.samples);
        glUniform1i(loc_iChannel0, 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, fraktal_get_gl_handle(in));

        fraktal_zero_array(out);
        fraktal_run_kernel(out);
    }

    fraktal_use_kernel(NULL);
}

void render_geometry(guiState &scene)
{
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

        for (int i = 0; i < scene.params.num_widgets; i++)
            scene.params.widgets[i]->set_params();

        fraktal_zero_array(out);
        fraktal_run_kernel(out);
        scene.samples = 0;
        scene.should_clear = false;
    }
    fraktal_use_kernel(NULL);
}

bool open_file_dialog(bool should_open, const char *label, char *buffer, size_t sizeof_buffer)
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

void gui_present(guiState &scene)
{
    static int last_mode = scene.mode;
    static bool open_model_succeeded = false;
    static bool open_color_succeeded = false;
    bool mode_changed = scene.mode != last_mode;
    last_mode = scene.mode;

    bool reload_key = scene.keys.Shift.down && scene.keys.Enter.pressed;
    if (reload_key || mode_changed || open_model_succeeded || open_color_succeeded)
    {
        open_model_succeeded = false;
        open_color_succeeded = false;
        log_clear();
        if (!gui_load(scene, scene.def))
            scene.got_error = true;
        else
            scene.got_error = false;
    }

    if (!scene.keys.Shift.down && scene.keys.Enter.pressed)
        scene.auto_render = !scene.auto_render;

    if (scene.mode == guiPreviewMode_Color)
    {
        if (scene.auto_render || scene.should_clear)
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
                if (ImGui::MenuItem("Load model kernel##MainMenu")) open_model_popup = true;
                if (ImGui::MenuItem("Load color kernel##MainMenu")) open_color_popup = true;
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
                ImGui::BulletText("Shift+Enter");        ImGui::NextColumn(); ImGui::Text("Reload kernel"); ImGui::NextColumn();
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
        float avail_y = io.DisplaySize.y - main_menu_bar_height - pad - pad;
        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse;
        ImGui::SetNextWindowSize(ImVec2(0.3f*io.DisplaySize.x, avail_y), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, avail_y), ImVec2(io.DisplaySize.x, avail_y));
        ImGui::SetNextWindowPos(ImVec2(pad, main_menu_bar_height + pad));

        ImGui::Begin("Scene parameters", NULL, flags);
        side_panel_width = ImGui::GetWindowWidth();
        side_panel_height = ImGui::GetWindowHeight();

        if (ImGui::CollapsingHeader("Resolution"))
        {
            ImGui::InputInt("x##resolution", &scene.params.resolution.x);
            ImGui::InputInt("y##resolution", &scene.params.resolution.y);
        }

        for (int i = 0; i < scene.params.num_widgets; i++)
            scene.should_clear |= scene.params.widgets[i]->update(scene);

        ImGui::End();
    }

    // tabs in main menu bar
    {
        ImGui::BeginMainMenuBar();
        ImGui::SetCursorPosX(side_panel_width + pad + pad + 8.0f);
        if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_None))
        {
            if (ImGui::BeginTabItem("Color"))     { scene.mode = guiPreviewMode_Color; ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Thickness")) { scene.mode = guiPreviewMode_Thickness; ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Normals"))   { scene.mode = guiPreviewMode_Normals; ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Depth"))     { scene.mode = guiPreviewMode_Depth; ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("GBuffer"))   { scene.mode = guiPreviewMode_GBuffer; ImGui::EndTabItem(); }
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
        ImGui::SetNextWindowPos(ImVec2(side_panel_width + pad + pad, main_menu_bar_height + pad));
        ImGui::Begin("Preview", NULL, flags);

        {
            const int display_mode_1x = 0;
            const int display_mode_2x = 1;
            const int display_mode_fit = 2;
            static int display_mode = display_mode_1x;

            ImGui::BeginMenuBar();
            {
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f,4.0f));
                if (ImGui::BeginMenu("Scale"))
                {
                    if (ImGui::MenuItem("1x", NULL, display_mode==display_mode_1x)) { display_mode = display_mode_1x; }
                    if (ImGui::MenuItem("2x", NULL, display_mode==display_mode_2x)) { display_mode = display_mode_2x; }
                    if (ImGui::MenuItem("Scale to fit", NULL, display_mode==display_mode_fit)) { display_mode = display_mode_fit; }
                    ImGui::EndMenu();
                }
                ImGui::PopStyleVar();
                ImGui::Separator();
                ImGui::Text("Samples: %d", scene.samples);
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
                    int num_checkers_x = 8;
                    int num_checkers_y = 8;
                    float checker_size_x = (pos1.x - pos0.x) / num_checkers_x;
                    float checker_size_y = (pos1.y - pos0.y) / num_checkers_y;
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
        open_model_succeeded = open_file_dialog(open_model_popup, "Open model kernel##Popup", path, sizeof(path));
        if (open_model_succeeded)
            scene.def.model_kernel_path = path;
    }
    {
        static char path[1024];
        open_color_succeeded = open_file_dialog(open_color_popup, "Open color kernel##Popup", path, sizeof(path));
        if (open_color_succeeded)
            scene.def.color_kernel_path = path;
    }
    {
        static char path[1024];
        if (open_file_dialog(open_screenshot_popup, "Save as PNG##Popup", path, sizeof(path)))
            save_screenshot(path, scene.compose_buffer);
    }

    ImGui::PopStyleVar(pushed_style_var);
    ImGui::PopStyleColor(pushed_style_col);
}
