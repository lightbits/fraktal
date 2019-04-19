#pragma once

struct Widget_Geometry : Widget
{
    float min_distance;
    float max_distance;
    int loc_iMinDistance;
    int loc_iMaxDistance;

    Widget_Geometry(guiSceneParams *params, const char **cc)
    {
        min_distance = 10.0f;
        max_distance = 30.0f;

        while (parse_next_in_list(cc)) {
            if (parse_argument_float(cc, "min_distance", &min_distance)) ;
            else if (parse_argument_float(cc, "max_distance", &max_distance)) ;
            else parse_list_unexpected();
        }
    }
    virtual void get_param_offsets(fKernel *f)
    {
        loc_iMinDistance = fraktal_get_param_offset(f, "iMinDistance");
        loc_iMaxDistance = fraktal_get_param_offset(f, "iMaxDistance");
    }
    virtual bool update(guiState g)
    {
        bool changed = false;
        if (ImGui::CollapsingHeader("Geometry"))
        {
            changed |= ImGui::DragFloat("Min. depth", &min_distance, 0.1f);
            changed |= ImGui::DragFloat("Max. depth", &max_distance, 0.1f);
        }
        return changed;
    }
    virtual void set_params()
    {
        fraktal_param_1f(loc_iMinDistance, min_distance);
        fraktal_param_1f(loc_iMaxDistance, max_distance);
    }
};
