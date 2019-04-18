// Developed by Simen Haugo.
// See LICENSE.txt for copyright and licensing details (standard MIT License).

#include "fraktal.h"
#include "fraktal_parse.h"
#include "gui.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>
#include <stdio.h>
#include <stdlib.h> // malloc, free
#include <string.h>
#include <GL/gl3w.h>
#include <file.h>

struct fraktal_scene_t
{
    fraktal_scene_def_t def;
    fArray *render_buffer;
    fArray *compose_buffer;
    fKernel *render_kernel;
    fKernel *compose_kernel;
    bool render_kernel_is_new;
    bool compose_kernel_is_new;
    int samples;
    bool should_clear;
    bool initialized;
    bool auto_render;
    GLuint quad;

    scene_params_t params;

    struct key_t
    {
        bool pressed;
        bool released;
        bool down;
    };
    struct keys_t
    {
        key_t Space,Enter;
        key_t Ctrl,Alt,Shift;
        key_t Left,Right,Up,Down;
        key_t W,A,S,D;
    } keys;
};

void compute_view_matrix(float dst[4*4], float3 t, float3 r)
{
    float cx = cosf(r.x);
    float cy = cosf(r.y);
    float cz = cosf(r.z);
    float sx = sinf(r.x);
    float sy = sinf(r.y);
    float sz = sinf(r.z);

    float dtx = t.z*(sx*sz + cx*cz*sy) - t.y*(cx*sz - cz*sx*sy) + t.x*cy*cz;
    float dty = t.y*(cx*cz + sx*sy*sz) - t.z*(cz*sx - cx*sy*sz) + t.x*cy*sz;
    float dtz = t.z*cx*cy              - t.x*sy                 + t.y*cy*sx;

    // T(tx,ty,tz)Rz(rz)Ry(ry)Rx(rx)
    dst[ 0] = cy*cz; dst[ 1] = cz*sx*sy - cx*sz; dst[ 2] = sx*sz + cx*cz*sy; dst[ 3] = dtx;
    dst[ 4] = cy*sz; dst[ 5] = cx*cz + sx*sy*sz; dst[ 6] = cx*sy*sz - cz*sx; dst[ 7] = dty;
    dst[ 8] = -sy;   dst[ 9] = cy*sx;            dst[10] = cx*cy;            dst[11] = dtz;
    dst[12] = 0.0f;  dst[13] = 0.0f;             dst[14] = 0.0f;             dst[15] = 0.0f;
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

void fraktal_set_resolution(fraktal_scene_t &scene, int x, int y)
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

bool fraktal_load(fraktal_scene_t &scene,
                  fraktal_scene_def_t def,
                  fraktal_load_flags_t flags)
{
    if (!scene.initialized)
        scene.params = get_default_scene_params();

    if (flags == 0)
        flags = 0xffffffff;

    scene_params_t params = scene.params;

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
            fraktal_set_resolution(scene, params.resolution.x, params.resolution.y);

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

void fraktal_render(fraktal_scene_t &scene)
{
    assert(scene.render_buffer);
    assert(fraktal_is_valid_array(scene.render_buffer));
    assert(fraktal_is_valid_array(scene.compose_buffer));

    fraktal_use_kernel(scene.render_kernel);
    fetch_uniform(render_kernel, iResolution);
    fetch_uniform(render_kernel, iCameraCenter);
    fetch_uniform(render_kernel, iCameraF);
    fetch_uniform(render_kernel, iSamples);
    fetch_uniform(render_kernel, iToSun);
    fetch_uniform(render_kernel, iSunStrength);
    fetch_uniform(render_kernel, iCosSunSize);
    fetch_uniform(render_kernel, iDrawIsolines);
    fetch_uniform(render_kernel, iIsolineColor);
    fetch_uniform(render_kernel, iIsolineThickness);
    fetch_uniform(render_kernel, iIsolineSpacing);
    fetch_uniform(render_kernel, iIsolineMax);
    fetch_uniform(render_kernel, iMaterialGlossy);
    fetch_uniform(render_kernel, iMaterialSpecularExponent);
    fetch_uniform(render_kernel, iMaterialSpecularAlbedo);
    fetch_uniform(render_kernel, iMaterialAlbedo);
    fetch_uniform(render_kernel, iFloorReflective);
    fetch_uniform(render_kernel, iFloorHeight);
    fetch_uniform(render_kernel, iFloorSpecularExponent);
    fetch_uniform(render_kernel, iFloorReflectivity);
    fetch_uniform(render_kernel, iView);
    scene.render_kernel_is_new = false;

    fArray *out = scene.render_buffer;

    int width,height;
    fraktal_get_array_size(out, &width, &height);
    glUniform2f(loc_iResolution, (float)width, (float)height);
    glUniform2fv(loc_iCameraCenter, 1, &scene.params.camera.center.x);
    glUniform1f(loc_iCameraF, scene.params.camera.f);
    glUniform1i(loc_iSamples, scene.samples);

    {
        float iView[4*4];
        float3 r = {
            deg2rad(scene.params.view.dir.theta),
            deg2rad(scene.params.view.dir.phi),
            0.0f
        };
        compute_view_matrix(iView, scene.params.view.pos, r);
        glUniformMatrix4fv(loc_iView, 1, GL_TRUE, iView);
    }

    {
        auto sun = scene.params.sun;
        float3 iSunStrength = sun.color;
        iSunStrength.x *= sun.intensity;
        iSunStrength.y *= sun.intensity;
        iSunStrength.z *= sun.intensity;
        float3 iToSun = angle2float3(sun.dir);
        float iCosSunSize = cosf(deg2rad(sun.size));
        glUniform3fv(loc_iSunStrength, 1, &iSunStrength.x);
        glUniform3fv(loc_iToSun, 1, &iToSun.x);
        glUniform1f(loc_iCosSunSize, iCosSunSize);
    }

    {
        auto iso = scene.params.isolines;
        glUniform1i(loc_iDrawIsolines, iso.enabled ? 1 : 0);
        glUniform3fv(loc_iIsolineColor, 1, &iso.color.x);
        glUniform1f(loc_iIsolineThickness, iso.thickness);
        glUniform1f(loc_iIsolineSpacing, iso.spacing);
        glUniform1f(loc_iIsolineMax, iso.count*iso.spacing + iso.thickness*0.5f);
    }

    {
        auto material = scene.params.material;
        glUniform1i(loc_iMaterialGlossy, material.glossy ? 1 : 0);
        glUniform1f(loc_iMaterialSpecularExponent, material.specular_exponent);
        glUniform3fv(loc_iMaterialSpecularAlbedo, 1, &material.specular_albedo.x);
        glUniform3fv(loc_iMaterialAlbedo, 1, &material.albedo.x);
    }

    {
        auto floor = scene.params.floor;
        glUniform1i(loc_iFloorReflective, floor.reflective ? 1 : 0);
        glUniform1f(loc_iFloorHeight, floor.height);
        glUniform1f(loc_iFloorSpecularExponent, floor.specular_exponent);
        glUniform1f(loc_iFloorReflectivity, floor.reflectivity);
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

void fraktal_compose(fraktal_scene_t &scene)
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

bool handle_view_change_keys(fraktal_scene_t &scene)
{
    bool moved = false;

    // rotation
    int rotate_step = 5;
    angle2 &dir = scene.params.view.dir;
    if (scene.keys.Left.pressed)  { dir.phi   -= rotate_step; moved = true; }
    if (scene.keys.Right.pressed) { dir.phi   += rotate_step; moved = true; }
    if (scene.keys.Up.pressed)    { dir.theta -= rotate_step; moved = true; }
    if (scene.keys.Down.pressed)  { dir.theta += rotate_step; moved = true; }

    // translation
    // Note: The z_over_f factor ensures that a key press yields the same
    // displacement of the object in image pixels, irregardless of how far
    // away the camera is.
    float3 &pos = scene.params.view.pos;
    float z_over_f = fabsf(pos.z)/scene.params.camera.f;
    float x_move_step = (scene.params.resolution.x*0.05f)*z_over_f;
    float y_move_step = (scene.params.resolution.y*0.05f)*z_over_f;
    float z_move_step = 0.1f*fabsf(scene.params.view.pos.z);
    if (scene.keys.Ctrl.pressed)  { pos.y -= y_move_step; moved = true; }
    if (scene.keys.Space.pressed) { pos.y += y_move_step; moved = true; }
    if (scene.keys.A.pressed)     { pos.x -= x_move_step; moved = true; }
    if (scene.keys.D.pressed)     { pos.x += x_move_step; moved = true; }
    if (scene.keys.W.pressed)     { pos.z -= z_move_step; moved = true; }
    if (scene.keys.S.pressed)     { pos.z += z_move_step; moved = true; }
    return moved;
}

void fraktal_present(fraktal_scene_t &scene)
{
    if (scene.keys.Shift.down && scene.keys.Enter.pressed)
        fraktal_load(scene, scene.def, FRAKTAL_LOAD_RENDER|FRAKTAL_LOAD_COMPOSE);

    if (handle_view_change_keys(scene))
        scene.should_clear = true;

    if (!scene.keys.Shift.down && scene.keys.Enter.pressed)
        scene.auto_render = !scene.auto_render;

    if (scene.auto_render || scene.render_kernel_is_new || scene.should_clear)
    {
        fraktal_render(scene);
        fraktal_compose(scene);
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
                if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    float3 &pos = scene.params.view.pos;
                    float z_over_f = fabsf(pos.z)/scene.params.camera.f;
                    float3 drag_speeds;
                    drag_speeds.x = (scene.params.resolution.x*0.005f)*z_over_f;
                    drag_speeds.y = (scene.params.resolution.y*0.005f)*z_over_f;
                    drag_speeds.z = 0.01f*fabsf(scene.params.view.pos.z);

                    scene.should_clear |= ImGui::SliderFloat("\xce\xb8##view_dir", &scene.params.view.dir.theta, -90.0f, +90.0f, "%.0f deg");
                    scene.should_clear |= ImGui::SliderFloat("\xcf\x86##view_dir", &scene.params.view.dir.phi, -180.0f, +180.0f, "%.0f deg");
                    scene.should_clear |= ImGui::DragFloat3("pos##view_pos", &pos.x, &drag_speeds.x);
                    scene.should_clear |= ImGui::DragFloat("f##camera_f", &scene.params.camera.f);
                    scene.should_clear |= ImGui::DragFloat2("center##camera_center", &scene.params.camera.center.x);
                }
                if (ImGui::CollapsingHeader("Sun"))
                {
                    scene.should_clear |= ImGui::SliderFloat("size##sun_size", &scene.params.sun.size, 0.0f, 180.0f, "%.0f deg");
                    scene.should_clear |= ImGui::SliderFloat("\xce\xb8##sun_dir", &scene.params.sun.dir.theta, -90.0f, +90.0f, "%.0f deg");
                    scene.should_clear |= ImGui::SliderFloat("\xcf\x86##sun_dir", &scene.params.sun.dir.phi, -180.0f, +180.0f, "%.0f deg");
                    scene.should_clear |= ImGui::SliderFloat3("color##sun_color", &scene.params.sun.color.x, 0.0f, 1.0f);
                    scene.should_clear |= ImGui::DragFloat("intensity##sun_intensity", &scene.params.sun.intensity);
                }
                if (ImGui::CollapsingHeader("Floor"))
                {
                    auto &floor = scene.params.floor;
                    scene.should_clear |= ImGui::DragFloat("Height", &floor.height, 0.01f);

                    if (ImGui::TreeNode("Isolines"))
                    {
                        auto &isolines = scene.params.isolines;
                        scene.should_clear |= ImGui::Checkbox("Enabled##Isolines", &isolines.enabled);
                        scene.should_clear |= ImGui::ColorEdit3("Color", &isolines.color.x);
                        scene.should_clear |= ImGui::DragFloat("Thickness", &isolines.thickness, 0.01f);
                        scene.should_clear |= ImGui::DragFloat("Spacing", &isolines.spacing, 0.01f);
                        scene.should_clear |= ImGui::DragInt("Count", &isolines.count, 0.1f, 0, 100);
                        ImGui::TreePop();
                    }

                    if (ImGui::TreeNode("Reflection"))
                    {
                        scene.should_clear |= ImGui::Checkbox("Enabled##Reflection", &floor.reflective);
                        scene.should_clear |= ImGui::DragFloat("Exponent", &floor.specular_exponent, 1.0f, 0.0f, 10000.0f);
                        scene.should_clear |= ImGui::SliderFloat("Reflectivity", &floor.reflectivity, 0.0f, 1.0f);
                        ImGui::TreePop();
                    }
                }
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