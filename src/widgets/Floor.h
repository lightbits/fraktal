#pragma once

struct Widget_Floor : Widget
{
    bool isolines_enabled;
    float3 isolines_color;
    float isolines_thickness;
    float isolines_spacing;
    int isolines_count;
    bool floor_reflective;
    float floor_height;
    float floor_specular_exponent;
    float floor_reflectivity;

    int loc_iDrawIsolines;
    int loc_iIsolineColor;
    int loc_iIsolineThickness;
    int loc_iIsolineSpacing;
    int loc_iIsolineMax;
    int loc_iFloorReflective;
    int loc_iFloorHeight;
    int loc_iFloorSpecularExponent;
    int loc_iFloorReflectivity;

    Widget_Floor(guiSceneParams *params, const char **cc)
    {
        isolines_enabled = false;
        isolines_color.x = 0.3f;
        isolines_color.y = 0.3f;
        isolines_color.z = 0.3f;
        isolines_thickness = 0.25f*0.5f;
        isolines_spacing = 0.4f;
        isolines_count = 3;
        floor_reflective = false;
        floor_height = 0.0f;
        floor_specular_exponent = 500.0f;
        floor_reflectivity = 0.6f;

        while (parse_next_in_list(cc)) {
            if (parse_argument_float3(cc, "isolines_color", &isolines_color)) ;
            else if (parse_argument_float(cc, "isolines_thickness", &isolines_thickness)) ;
            else if (parse_argument_float(cc, "isolines_spacing", &isolines_spacing)) ;
            else if (parse_argument_int(cc, "isolines_count", &isolines_count)) ;
            else if (parse_argument_float(cc, "height", &floor_height)) ;
            else if (parse_argument_float(cc, "specular_exponent", &floor_specular_exponent)) ;
            else if (parse_argument_float(cc, "reflectivity", &floor_reflectivity)) ;
            else parse_list_unexpected();
        }
    }
    virtual void get_param_offsets(fKernel *f)
    {
        loc_iDrawIsolines = fraktal_get_param_offset(f, "iDrawIsolines");
        loc_iIsolineColor = fraktal_get_param_offset(f, "iIsolineColor");
        loc_iIsolineThickness = fraktal_get_param_offset(f, "iIsolineThickness");
        loc_iIsolineSpacing = fraktal_get_param_offset(f, "iIsolineSpacing");
        loc_iIsolineMax = fraktal_get_param_offset(f, "iIsolineMax");
        loc_iFloorReflective = fraktal_get_param_offset(f, "iFloorReflective");
        loc_iFloorHeight = fraktal_get_param_offset(f, "iFloorHeight");
        loc_iFloorSpecularExponent = fraktal_get_param_offset(f, "iFloorSpecularExponent");
        loc_iFloorReflectivity = fraktal_get_param_offset(f, "iFloorReflectivity");
    }
    virtual bool update(guiState g)
    {
        bool changed = false;
        if (ImGui::CollapsingHeader("Floor"))
        {
            changed |= ImGui::DragFloat("Height", &floor_height, 0.01f);

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
                changed |= ImGui::Checkbox("Enabled##Reflection", &floor_reflective);
                changed |= ImGui::DragFloat("Exponent", &floor_specular_exponent, 1.0f, 0.0f, 10000.0f);
                changed |= ImGui::SliderFloat("Reflectivity", &floor_reflectivity, 0.0f, 1.0f);
                ImGui::TreePop();
            }
        }
        return changed;
    }
    virtual void set_params()
    {
        fraktal_param_1i(loc_iDrawIsolines, isolines_enabled ? 1 : 0);
        fraktal_param_3f(loc_iIsolineColor, isolines_color.x, isolines_color.y, isolines_color.z);
        fraktal_param_1f(loc_iIsolineThickness, isolines_thickness);
        fraktal_param_1f(loc_iIsolineSpacing, isolines_spacing);
        fraktal_param_1f(loc_iIsolineMax, isolines_count*isolines_spacing + isolines_thickness*0.5f);
        fraktal_param_1i(loc_iFloorReflective, floor_reflective ? 1 : 0);
        fraktal_param_1f(loc_iFloorHeight, floor_height);
        fraktal_param_1f(loc_iFloorSpecularExponent, floor_specular_exponent);
        fraktal_param_1f(loc_iFloorReflectivity, floor_reflectivity);
    }
};
