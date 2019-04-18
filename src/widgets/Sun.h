#pragma once

struct Widget_Sun : Widget
{
    float size;
    angle2 dir;
    float3 color;
    float intensity;
    int loc_iSunStrength;
    int loc_iCosSunSize;
    int loc_iToSun;

    Widget_Sun(guiSceneParams *params, const char **cc)
    {
        size = 3.0f;
        dir.theta = 30.0f;
        dir.phi = 90.0f;
        color.x = 1.0f;
        color.y = 1.0f;
        color.z = 0.8f;
        intensity = 250.0f;

        while (parse_next_in_list(cc)) {
            if (parse_argument_angle(cc, "size", &size)) ;
            else if (parse_argument_angle2(cc, "dir", &dir)) ;
            else if (parse_argument_float3(cc, "color", &color)) ;
            else if (parse_argument_float(cc, "intensity", &intensity)) ;
            else parse_list_unexpected();
        }
    }
    virtual void get_param_offsets(fKernel *f)
    {
        loc_iSunStrength = fraktal_get_param_offset(f, "iSunStrength");
        loc_iCosSunSize = fraktal_get_param_offset(f, "iCosSunSize");
        loc_iToSun = fraktal_get_param_offset(f, "iToSun");
    }
    virtual bool update(guiState g)
    {
        bool changed = false;
        if (ImGui::CollapsingHeader("Sun"))
        {
            if (loc_iCosSunSize != -1)
            {
                changed |= ImGui::SliderFloat("size##sun_size", &size, 0.0f, 180.0f, "%.0f deg");
            }
            if (loc_iToSun != -1)
            {
                changed |= ImGui::SliderFloat("\xce\xb8##sun_dir", &dir.theta, -90.0f, +90.0f, "%.0f deg");
                changed |= ImGui::SliderFloat("\xcf\x86##sun_dir", &dir.phi, -180.0f, +180.0f, "%.0f deg");
            }
            if (loc_iSunStrength != -1)
            {
                changed |= ImGui::SliderFloat3("color##sun_color", &color.x, 0.0f, 1.0f);
                changed |= ImGui::DragFloat("intensity##sun_intensity", &intensity);
            }
        }
        return changed;
    }
    virtual void set_params()
    {
        float3 iSunStrength = color;
        iSunStrength.x *= intensity;
        iSunStrength.y *= intensity;
        iSunStrength.z *= intensity;
        float3 iToSun = angle2float3(dir);
        float iCosSunSize = cosf(deg2rad(size));
        fraktal_param_3f(loc_iSunStrength, iSunStrength.x, iSunStrength.y, iSunStrength.z);
        fraktal_param_3f(loc_iToSun, iToSun.x, iToSun.y, iToSun.z);
        fraktal_param_1f(loc_iCosSunSize, iCosSunSize);
    }
};
