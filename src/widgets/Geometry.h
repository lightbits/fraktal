#pragma once
#include "colormap_inferno.h"

static fArray *f_colormap_inferno;

struct Widget_Geometry : Widget
{
    float min_distance;
    float max_distance;
    float min_thickness;
    float max_thickness;
    bool apply_colormap;
    int loc_iMinDistance;
    int loc_iMaxDistance;
    int loc_iMinThickness;
    int loc_iMaxThickness;
    int loc_iApplyColormap;
    int loc_iColormap;

    Widget_Geometry(guiSceneParams *params, const char **cc)
    {
        min_distance = 10.0f;
        max_distance = 30.0f;
        min_thickness = 0.0f;
        max_thickness = 0.5f;
        apply_colormap = false;

        if (!f_colormap_inferno)
        {
            f_colormap_inferno = fraktal_create_array(
                colormap_inferno,
                colormap_inferno_length,
                0,
                4,
                FRAKTAL_FLOAT,
                FRAKTAL_READ_ONLY
            );
            assert(f_colormap_inferno);
        }

        while (parse_next_in_list(cc)) {
            if (parse_argument_float(cc, "min_distance", &min_distance)) ;
            else if (parse_argument_float(cc, "max_distance", &max_distance)) ;
            else if (parse_argument_float(cc, "min_thickness", &min_thickness)) ;
            else if (parse_argument_float(cc, "max_thickness", &max_thickness)) ;
            else if (parse_argument_bool(cc, "apply_colormap", &apply_colormap)) ;
            else parse_list_unexpected();
        }
    }
    virtual void get_param_offsets(fKernel *f)
    {
        loc_iMinDistance = fraktal_get_param_offset(f, "iMinDistance");
        loc_iMaxDistance = fraktal_get_param_offset(f, "iMaxDistance");
        loc_iMinThickness = fraktal_get_param_offset(f, "iMinThickness");
        loc_iMaxThickness = fraktal_get_param_offset(f, "iMaxThickness");
        loc_iApplyColormap = fraktal_get_param_offset(f, "iApplyColormap");
        loc_iColormap = fraktal_get_param_offset(f, "iColormap");
    }
    virtual bool update(guiState g)
    {
        bool changed = false;
        if (ImGui::CollapsingHeader("Geometry"))
        {
            changed |= ImGui::Checkbox("Apply colormap", &apply_colormap);
            changed |= ImGui::DragFloat("Min. depth", &min_distance, 0.1f);
            changed |= ImGui::DragFloat("Max. depth", &max_distance, 0.1f);
            changed |= ImGui::DragFloat("Min. thickness", &min_thickness, 0.1f);
            changed |= ImGui::DragFloat("Max. thickness", &max_thickness, 0.1f);
        }
        return changed;
    }
    virtual void set_params()
    {
        fraktal_param_1f(loc_iMinDistance, min_distance);
        fraktal_param_1f(loc_iMaxDistance, max_distance);
        fraktal_param_1f(loc_iMinThickness, min_thickness);
        fraktal_param_1f(loc_iMaxThickness, max_thickness);
        fraktal_param_1i(loc_iApplyColormap, apply_colormap ? 1 : 0);
        fraktal_param_array(loc_iColormap, 0, f_colormap_inferno);
    }
};
