#pragma once

struct Widget_Material : Widget
{
    bool glossy;
    float3 specular_albedo;
    float specular_exponent;
    float3 albedo;
    int loc_iMaterialGlossy;
    int loc_iMaterialSpecularExponent;
    int loc_iMaterialSpecularAlbedo;
    int loc_iMaterialAlbedo;

    Widget_Material(guiSceneParams *params, const char **cc)
    {
        glossy = true;
        specular_albedo.x = 0.3f;
        specular_albedo.y = 0.3f;
        specular_albedo.z = 0.3f;
        specular_exponent = 32.0f;
        albedo.x = 0.6f;
        albedo.y = 0.1f;
        albedo.z = 0.1f;

        while (parse_next_in_list(cc)) {
            if (parse_argument_float3(cc, "albedo", &albedo)) ;
            else if (parse_argument_float3(cc, "specular_albedo", &specular_albedo)) ;
            else if (parse_argument_float(cc, "specular_exponent", &specular_exponent)) ;
            else if (parse_argument_bool(cc, "glossy", &glossy)) ;
            else parse_list_unexpected();
        }
    }
    virtual void get_param_offsets(fKernel *f)
    {
        loc_iMaterialGlossy = fraktal_get_param_offset(f, "iMaterialGlossy");
        loc_iMaterialSpecularExponent = fraktal_get_param_offset(f, "iMaterialSpecularExponent");
        loc_iMaterialSpecularAlbedo = fraktal_get_param_offset(f, "iMaterialSpecularAlbedo");
        loc_iMaterialAlbedo = fraktal_get_param_offset(f, "iMaterialAlbedo");
    }
    virtual bool update(guiState g)
    {
        bool changed = false;
        if (ImGui::CollapsingHeader("Material"))
        {
            changed |= ImGui::Checkbox("Glossy", &glossy);
            changed |= ImGui::ColorEdit3("Albedo", &albedo.x);
            changed |= ImGui::ColorEdit3("Specular", &specular_albedo.x);
            changed |= ImGui::DragFloat("Exponent", &specular_exponent, 1.0f, 0.0f, 1000.0f);
        }
        return changed;
    }
    virtual void set_params()
    {
        fraktal_param_1i(loc_iMaterialGlossy, glossy ? 1 : 0);
        fraktal_param_1f(loc_iMaterialSpecularExponent, specular_exponent);
        fraktal_param_3f(loc_iMaterialSpecularAlbedo, specular_albedo.x, specular_albedo.y, specular_albedo.z);
        fraktal_param_3f(loc_iMaterialAlbedo, albedo.x, albedo.y, albedo.z);
    }
};
