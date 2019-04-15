#include <imgui_internal.h>

namespace ImGui
{
    // Modification of DragScalarN to support per-component speeds
    bool DragScalarN(const char* label, ImGuiDataType data_type, void* v, int components, float* v_speed, const void* v_min, const void* v_max, const char* format, float power)
    {
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiContext& g = *GImGui;
        bool value_changed = false;
        BeginGroup();
        PushID(label);
        PushMultiItemsWidths(components);
        size_t type_size = GDataTypeInfo[data_type].Size;
        for (int i = 0; i < components; i++)
        {
            PushID(i);
            value_changed |= DragScalar("", data_type, v, v_speed[i], v_min, v_max, format, power);
            SameLine(0, g.Style.ItemInnerSpacing.x);
            PopID();
            PopItemWidth();
            v = (void*)((char*)v + type_size);
        }
        PopID();

        TextEx(label, FindRenderedTextEnd(label));
        EndGroup();
        return value_changed;
    }

    // v_speed = NULL -> use 1.0f
    bool DragFloat3(const char* label, float v[3], float* v_speed = NULL, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", float power = 1.0f)
    {
        if (v_speed)
            return DragScalarN(label, ImGuiDataType_Float, v, 3, v_speed, &v_min, &v_max, format, power);
        else
            return DragScalarN(label, ImGuiDataType_Float, v, 3, 1.0f, &v_min, &v_max, format, power);
    }
}
