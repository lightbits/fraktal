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

    virtual void default_values()
    {
        size = 3.0f;
        dir.theta = 30.0f;
        dir.phi = 90.0f;
        color.x = 1.0f;
        color.y = 1.0f;
        color.z = 0.8f;
        intensity = 250.0f;
    }
    virtual void deserialize(const char **cc)
    {
        while (parse_next_in_list(cc)) {
            if (parse_argument_angle(cc, "size", &size)) ;
            else if (parse_argument_angle2(cc, "dir", &dir)) ;
            else if (parse_argument_float3(cc, "color", &color)) ;
            else if (parse_argument_float(cc, "intensity", &intensity)) ;
            else parse_list_unexpected();
        }
    }
    virtual void serialize(FILE *f)
    {

    }
    virtual void get_param_offsets(fKernel *f)
    {
        loc_iSunStrength = fraktal_get_param_offset(f, "iSunStrength");
        loc_iCosSunSize = fraktal_get_param_offset(f, "iCosSunSize");
        loc_iToSun = fraktal_get_param_offset(f, "iToSun");
    }
    virtual bool is_active()
    {
        if (loc_iCosSunSize < 0) return false;
        if (loc_iToSun < 0) return false;
        return true;
    }
    virtual bool update(guiState &g)
    {
        bool changed = false;
        if (ImGui::CollapsingHeader("Sun"))
        {
            changed |= ImGui::SliderFloat("Size##sun_size", &size, 0.0f, 180.0f, "%.0f deg", 2.0f);
            changed |= ImGui::SliderFloat("\xce\xb8##sun_dir", &dir.theta, -90.0f, +90.0f, "%.0f deg");
            changed |= ImGui::SliderFloat("\xcf\x86##sun_dir", &dir.phi, -180.0f, +180.0f, "%.0f deg");
            changed |= ImGui::SliderFloat3("Color##sun_color", &color.x, 0.0f, 1.0f);
            changed |= ImGui::DragFloat("Intensity##sun_intensity", &intensity);
        }
        return changed;
    }
    virtual void set_params(guiState &g)
    {
        float3 iSunStrength = color;
        iSunStrength.x *= intensity;
        iSunStrength.y *= intensity;
        iSunStrength.z *= intensity;
        float3 iToSun = angle2float3(dir);
        float iCosSunSize = cosf(deg2rad(size)/2.0f);
        fraktal_param_3f(loc_iSunStrength, iSunStrength.x, iSunStrength.y, iSunStrength.z);
        fraktal_param_3f(loc_iToSun, iToSun.x, iToSun.y, iToSun.z);
        fraktal_param_1f(loc_iCosSunSize, iCosSunSize);
    }
};
