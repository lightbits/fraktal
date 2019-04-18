// Developed by Simen Haugo.
// See LICENSE.txt for copyright and licensing details (standard MIT License).

#include "fraktal.h"
#include "fraktal_parse.h"
#include "gui_state.h"

typedef int guiLoadFlags;
enum guiLoadFlags_
{
    GUI_LOAD_ALL          = 0,
    GUI_LOAD_MODEL        = 1,
    GUI_LOAD_RENDER       = 2,
    GUI_LOAD_COMPOSE      = 4
};

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

bool parse_resolution(const char **c, guiSceneParams *params)
{
    parse_alpha(c);
    parse_blank(c);
    if (!parse_int2(c, &params->resolution)) { log_err("Error parsing #resolution(x,y) directive: expected int2 after token.\n"); return false; }
    return true;
}

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
                 if (parse_match(cc, "resolution")) { if (!parse_resolution(cc, params)) goto failure; }
            else if (parse_match(cc, "widget"))
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
                // ...
                // add new macros here!
                // ...
                { log_err("Error parsing #widget directive: unknown widget type.\n"); goto failure; }

                params->num_widgets++;
            }
            remove_directive_from_source(mark, c);
        }
        c++;
    }
    return true;

failure:
    for (int i = 0; i < params->num_widgets; i++)
        free(params->widgets[i]);
    return false;
}

guiSceneParams get_default_scene_params()
{
    guiSceneParams params = {0};
    params.resolution.x = 200;
    params.resolution.y = 200;
    params.material.albedo.x = 0.6f;
    params.material.albedo.y = 0.1f;
    params.material.albedo.z = 0.1f;
    params.material.glossy = true;
    params.material.specular_albedo.x = 0.3f;
    params.material.specular_albedo.y = 0.3f;
    params.material.specular_albedo.z = 0.3f;
    params.material.specular_exponent = 32.0f;
    return params;
}

#if 0
void save_screenshot(const char *filename)
{
    int w = app.fb_width;
    int h = app.fb_height;
    int n = 4;
    int stride = w*n;
    unsigned char *pixels = (unsigned char*)malloc(w*h*n);
    unsigned char *last_row = pixels + stride*(h-1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_FRONT);
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    // writing from last row with negative stride flips the image vertically
    stbi_write_png(filename, w, h, n, last_row, -stride);
    free(pixels);
}
#endif

void gui_set_resolution(guiState &scene, int x, int y)
{
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

bool gui_load(guiState &scene,
              guiSceneDef def,
              guiLoadFlags flags)
{
    if (!scene.initialized)
        scene.params = get_default_scene_params();

    if (flags == 0)
        flags = 0xffffffff;

    guiSceneParams params = scene.params;

    fKernel *render = NULL;
    {
        fLinkState *link = fraktal_create_link();

        // model kernel
        {
            const char *path = def.model_shader_path;
            char *data = read_file(path);
            if (!data)
            {
                log_err("Failed to read source file.\n");
                fraktal_destroy_link(link);
                return false;
            }
            if (!scene_file_preprocessor(data, &params))
            {
                log_err("Failed to parse source file.\n");
                fraktal_destroy_link(link);
                free(data);
                return false;
            }
            if (!fraktal_add_link_data(link, data, 0, path))
            {
                fraktal_destroy_link(link);
                free(data);
                return false;
            }
            free(data);
        }

        if (!fraktal_add_link_file(link, def.render_shader_path))
        {
            fraktal_destroy_link(link);
            return false;
        }

        render = fraktal_link_kernel(link);
        fraktal_destroy_link(link);
    }

    fKernel *compose = fraktal_load_kernel(def.compose_shader_path);

    if (compose && render)
    {
        bool resolution_changed =
            params.resolution.x != scene.params.resolution.x ||
            params.resolution.y != scene.params.resolution.y;

        if (!scene.initialized || resolution_changed)
            gui_set_resolution(scene, params.resolution.x, params.resolution.y);

        fraktal_destroy_kernel(scene.compose_kernel);
        fraktal_destroy_kernel(scene.render_kernel);

        scene.compose_kernel = compose;
        scene.render_kernel = render;
        scene.params = params;
        scene.compose_kernel_is_new = true;
        scene.render_kernel_is_new = true;
        scene.should_clear = true;
        scene.def = def;
        scene.initialized = true;

        for (int i = 0; i < scene.params.num_widgets; i++)
            scene.params.widgets[i]->get_param_offsets(scene.render_kernel);

        return true;
    }
    else
    {
        log_err("Failed to compile kernels\n");
        fraktal_destroy_kernel(render);
        fraktal_destroy_kernel(compose);
        return false;
    }

    return true;
}

#define fetch_uniform(kernel, name) static int loc_##name; if (scene.kernel##_is_new) loc_##name = fraktal_get_param_offset(scene.kernel, #name);

void render_scene(guiState &scene)
{
    assert(scene.render_buffer);
    assert(fraktal_is_valid_array(scene.render_buffer));
    assert(fraktal_is_valid_array(scene.compose_buffer));

    fraktal_use_kernel(scene.render_kernel);
    fetch_uniform(render_kernel, iResolution);
    fetch_uniform(render_kernel, iSamples);
    fetch_uniform(render_kernel, iMaterialGlossy);
    fetch_uniform(render_kernel, iMaterialSpecularExponent);
    fetch_uniform(render_kernel, iMaterialSpecularAlbedo);
    fetch_uniform(render_kernel, iMaterialAlbedo);
    scene.render_kernel_is_new = false;

    fArray *out = scene.render_buffer;

    int width,height;
    fraktal_get_array_size(out, &width, &height);
    glUniform2f(loc_iResolution, (float)width, (float)height);
    glUniform1i(loc_iSamples, scene.samples);

    for (int i = 0; i < scene.params.num_widgets; i++)
        scene.params.widgets[i]->set_params();

    {
        auto material = scene.params.material;
        glUniform1i(loc_iMaterialGlossy, material.glossy ? 1 : 0);
        glUniform1f(loc_iMaterialSpecularExponent, material.specular_exponent);
        glUniform3fv(loc_iMaterialSpecularAlbedo, 1, &material.specular_albedo.x);
        glUniform3fv(loc_iMaterialAlbedo, 1, &material.albedo.x);
    }

    if (scene.should_clear)
    {
        fraktal_zero_array(out);
        scene.samples = 0;
        scene.should_clear = false;
    }
    fraktal_run_kernel(out);
    fraktal_use_kernel(NULL);

    scene.samples++;
}

void compose_scene(guiState &scene)
{
    fraktal_use_kernel(scene.compose_kernel);

    fetch_uniform(compose_kernel, iResolution);
    fetch_uniform(compose_kernel, iChannel0);
    fetch_uniform(compose_kernel, iSamples);
    scene.compose_kernel_is_new = false;

    fArray *out = scene.compose_buffer;
    fArray *in = scene.render_buffer;
    int width,height;
    fraktal_get_array_size(out, &width, &height);
    glUniform2f(loc_iResolution, (float)width, (float)height);
    glUniform1i(loc_iSamples, scene.samples);
    glUniform1i(loc_iChannel0, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fraktal_get_gl_handle(in));

    fraktal_zero_array(out);
    fraktal_run_kernel(out);
    fraktal_use_kernel(NULL);
}

void gui_present(guiState &scene)
{
    if (scene.keys.Shift.down && scene.keys.Enter.pressed)
        gui_load(scene, scene.def, GUI_LOAD_RENDER|GUI_LOAD_COMPOSE);

    if (!scene.keys.Shift.down && scene.keys.Enter.pressed)
        scene.auto_render = !scene.auto_render;

    if (scene.auto_render || scene.render_kernel_is_new || scene.should_clear)
    {
        render_scene(scene);
        compose_scene(scene);
    }

    float pad = 2.0f;

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 4.0f);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 0.325f));
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(1.0f, 1.0f, 1.0f, 0.078f));
    ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(1.0f, 1.0f, 1.0f, 0.325f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 0.325f));
    // blue tabs
    // ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(1.0f, 1.0f, 1.0f, 0.114f));
    // ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(0.329f, 0.478f, 0.71f, 1.0f));
    // gray tabs
    ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.22f, 0.22f, 0.22f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(0.28f, 0.28f, 0.28f, 1.0f));

    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(1.0f, 1.0f, 1.0f, 0.325f));

    // main menu bar
    float main_menu_bar_height = 0.0f;
    int preview_mode_render = 0;
    int preview_mode_mesh = 1;
    int preview_mode_point_cloud = 2;
    int preview_mode = preview_mode_render;
    {
        ImGui::BeginMainMenuBar();
        {
            main_menu_bar_height = ImGui::GetWindowHeight();
            ImGui::Text("\xce\xb8"); // placeholder for Fraktal icon
            ImGui::MenuItem("File");
            ImGui::MenuItem("Window");
            ImGui::MenuItem("Help");
            if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_None))
            {
                if (ImGui::BeginTabItem("Render"))
                {
                    preview_mode = preview_mode_render;
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Mesh"))
                {
                    preview_mode = preview_mode_mesh;
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Point cloud"))
                {
                    preview_mode = preview_mode_point_cloud;
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
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
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoCollapse;
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.22f, 0.22f, 0.22f, 1.0f));
        ImGui::SetNextWindowSize(ImVec2(0.3f*io.DisplaySize.x, avail_y), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, avail_y), ImVec2(io.DisplaySize.x, avail_y));
        ImGui::SetNextWindowPos(ImVec2(0.0f, main_menu_bar_height + pad));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f,8.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 12.0f);
        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.0f,0.0f,0.0f,0.0f));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(1.0f,1.0f,1.0f,0.25f));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(1.0f,1.0f,1.0f,0.4f));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, ImVec4(1.0f,1.0f,1.0f,0.55f));
        ImGui::Begin("Sidepanel", NULL, flags);
        side_panel_width = ImGui::GetWindowWidth();
        side_panel_height = ImGui::GetWindowHeight();

        if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_None))
        {
            if (ImGui::BeginTabItem("Scene"))
            {
                ImGui::BeginChild("Scene##child");
                if (ImGui::CollapsingHeader("Resolution"))
                {
                    ImGui::InputInt("x##resolution", &scene.params.resolution.x);
                    ImGui::InputInt("y##resolution", &scene.params.resolution.y);
                }

                for (int i = 0; i < scene.params.num_widgets; i++)
                    scene.should_clear |= scene.params.widgets[i]->update(scene);

                if (ImGui::CollapsingHeader("Material"))
                {
                    auto &material = scene.params.material;
                    scene.should_clear |= ImGui::Checkbox("Glossy", &material.glossy);
                    scene.should_clear |= ImGui::ColorEdit3("Albedo", &material.albedo.x);
                    scene.should_clear |= ImGui::ColorEdit3("Specular", &material.specular_albedo.x);
                    scene.should_clear |= ImGui::DragFloat("Exponent", &material.specular_exponent, 1.0f, 0.0f, 1000.0f);
                }

                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Log"))
            {
                ImGui::BeginChild("Log##child");
                ImGui::TextWrapped(log_get_buffer());
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::End();
        ImGui::PopStyleVar();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
    }

    // main preview panel
    {
        ImGuiIO &io = ImGui::GetIO();
        float height = side_panel_height;
        float width = io.DisplaySize.x - (side_panel_width + pad);

        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_MenuBar;
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.14f, 0.14f, 0.14f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f,0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::SetNextWindowSize(ImVec2(width, height));
        ImGui::SetNextWindowPos(ImVec2(side_panel_width + pad, main_menu_bar_height + pad));
        ImGui::Begin("Preview", NULL, flags);

        if (preview_mode == preview_mode_render)
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
                fraktal_get_array_size(scene.compose_buffer, &width, &height);
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
                ImU32 tint = 0xFFFFFFFF;
                ImVec2 uv0 = ImVec2(0.0f,0.0f);
                ImVec2 uv1 = ImVec2(1.0f,1.0f);
                draw->AddImage((void*)(intptr_t)fraktal_get_gl_handle(scene.compose_buffer),
                               pos0, pos1, uv0, uv1, tint);
            }
        }

        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        ImGui::PopStyleVar();
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
}
