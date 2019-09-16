#pragma once

struct Widget_Ground : Widget
{
    bool isolines_enabled;
    float3 isolines_color;
    float isolines_thickness;
    float isolines_spacing;
    int isolines_count;
    bool ground_reflective;
    float ground_height;
    float ground_specular_exponent;
    float ground_reflectivity;

    int loc_iDrawIsolines;
    int loc_iIsolineColor;
    int loc_iIsolineThickness;
    int loc_iIsolineSpacing;
    int loc_iIsolineMax;
    int loc_iGroundReflective;
    int loc_iGroundHeight;
    int loc_iGroundSpecularExponent;
    int loc_iGroundReflectivity;

    virtual void default_values()
    {
        isolines_enabled = false;
        isolines_color.x = 0.3f;
        isolines_color.y = 0.3f;
        isolines_color.z = 0.3f;
        isolines_thickness = 0.25f*0.5f;
        isolines_spacing = 0.4f;
        isolines_count = 3;
        ground_reflective = false;
        ground_height = 0.0f;
        ground_specular_exponent = 500.0f;
        ground_reflectivity = 0.6f;

    }
    virtual void deserialize(const char **cc)
    {
        while (parse_next_in_list(cc)) {
            if (parse_argument_float3(cc, "isolines_color", &isolines_color)) ;
            else if (parse_argument_bool(cc, "draw_isolines", &isolines_enabled)) ;
            else if (parse_argument_float(cc, "isolines_thickness", &isolines_thickness)) ;
            else if (parse_argument_float(cc, "isolines_spacing", &isolines_spacing)) ;
            else if (parse_argument_int(cc, "isolines_count", &isolines_count)) ;
            else if (parse_argument_float(cc, "height", &ground_height)) ;
            else if (parse_argument_float(cc, "specular_exponent", &ground_specular_exponent)) ;
            else if (parse_argument_float(cc, "reflectivity", &ground_reflectivity)) ;
            else parse_list_unexpected();
        }
    }
    virtual void serialize(FILE *f)
    {

    }
    virtual void get_param_offsets(fKernel *f)
    {
        loc_iDrawIsolines = fraktal_get_param_offset(f, "iDrawIsolines");
        loc_iIsolineColor = fraktal_get_param_offset(f, "iIsolineColor");
        loc_iIsolineThickness = fraktal_get_param_offset(f, "iIsolineThickness");
        loc_iIsolineSpacing = fraktal_get_param_offset(f, "iIsolineSpacing");
        loc_iIsolineMax = fraktal_get_param_offset(f, "iIsolineMax");
        loc_iGroundReflective = fraktal_get_param_offset(f, "iGroundReflective");
        loc_iGroundHeight = fraktal_get_param_offset(f, "iGroundHeight");
        loc_iGroundSpecularExponent = fraktal_get_param_offset(f, "iGroundSpecularExponent");
        loc_iGroundReflectivity = fraktal_get_param_offset(f, "iGroundReflectivity");
    }
    virtual bool is_active()
    {
        if (loc_iDrawIsolines < 0) return false;
        if (loc_iIsolineColor < 0) return false;
        if (loc_iIsolineThickness < 0) return false;
        if (loc_iIsolineSpacing < 0) return false;
        if (loc_iIsolineMax < 0) return false;
        if (loc_iGroundReflective < 0) return false;
        if (loc_iGroundHeight < 0) return false;
        if (loc_iGroundSpecularExponent < 0) return false;
        if (loc_iGroundReflectivity < 0) return false;
        return true;
    }
    virtual bool update(guiState &g)
    {
        bool changed = false;
        if (ImGui::CollapsingHeader("Ground"))
        {
            changed |= ImGui::DragFloat("Height", &ground_height, 0.01f);

            if (ImGui::TreeNode("Isolines"))
            {
                changed |= ImGui::Checkbox("Enabled##Isolines", &isolines_enabled);
                changed |= ImGui::ColorEdit3("Color", &isolines_color.x);
                changed |= ImGui::DragFloat("Thickness", &isolines_thickness, 0.01f);
                changed |= ImGui::DragFloat("Spacing", &isolines_spacing, 0.01f);
                changed |= ImGui::DragInt("Count", &isolines_count, 0.1f, 0, 100);
                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Reflection"))
            {
                changed |= ImGui::Checkbox("Enabled##Reflection", &ground_reflective);
                changed |= ImGui::DragFloat("Exponent", &ground_specular_exponent, 1.0f, 0.0f, 10000.0f);
                changed |= ImGui::SliderFloat("Reflectivity", &ground_reflectivity, 0.0f, 1.0f);
                ImGui::TreePop();
            }
        }
        return changed;
    }
    virtual void set_params(guiState &g)
    {
        fraktal_param_1i(loc_iDrawIsolines, isolines_enabled ? 1 : 0);
        fraktal_param_3f(loc_iIsolineColor, isolines_color.x, isolines_color.y, isolines_color.z);
        fraktal_param_1f(loc_iIsolineThickness, isolines_thickness);
        fraktal_param_1f(loc_iIsolineSpacing, isolines_spacing);
        fraktal_param_1f(loc_iIsolineMax, isolines_count*isolines_spacing + isolines_thickness*0.5f);
        fraktal_param_1i(loc_iGroundReflective, ground_reflective ? 1 : 0);
        fraktal_param_1f(loc_iGroundHeight, ground_height);
        fraktal_param_1f(loc_iGroundSpecularExponent, ground_specular_exponent);
        fraktal_param_1f(loc_iGroundReflectivity, ground_reflectivity);
    }
};
