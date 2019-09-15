#pragma once

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

struct Widget_Camera : Widget
{
    angle2 dir;
    float3 pos;
    float camera_f;
    float2 camera_center;
    int loc_iView;
    int loc_iCameraF;
    int loc_iCameraCenter;

    virtual void default_values(guiState &g)
    {
        dir.theta = -20.0f;
        dir.phi = 30.0f;
        pos.x = 0.0f;
        pos.y = 0.0f;
        pos.z = 24.0f;
        camera_f = yfov2pinhole_f(10.0f, (float)g.resolution.y);
        camera_center.x = g.resolution.x/2.0f;
        camera_center.y = g.resolution.y/2.0f;
    }
    virtual void deserialize(const char **cc)
    {
        while (parse_next_in_list(cc)) {
            if (parse_argument_float(cc, "f", &camera_f)) ;
            else if (parse_argument_float2(cc, "center", &camera_center)) ;
            else if (parse_argument_angle2(cc, "dir", &dir)) ;
            else if (parse_argument_float3(cc, "pos", &pos)) ;
            else parse_list_unexpected();
        }
    }
    virtual void serialize(FILE *f)
    {
        fprintf(f, "camera=(");
        fprintf(f, "f=%f, ", camera_f);
        fprintf(f, "center=(%f, %f), ", camera_center.x, camera_center.y);
        fprintf(f, "dir=(%f deg, %f deg), ", dir.theta, dir.phi);
        fprintf(f, "pos=(%f, %f, %f))\n", pos.x, pos.y, pos.z);
    }
    virtual void get_param_offsets(fKernel *f)
    {
        loc_iView = fraktal_get_param_offset(f, "iView");
        loc_iCameraCenter = fraktal_get_param_offset(f, "iCameraCenter");
        loc_iCameraF = fraktal_get_param_offset(f, "iCameraF");
    }
    virtual bool is_active()
    {
        if (loc_iView < 0) return false;
        if (loc_iCameraCenter < 0) return false;
        if (loc_iCameraF < 0) return false;
        return true;
    }
    virtual bool update(guiState g)
    {
        bool changed = false;

        // rotation
        int rotate_step = 5;
        if (g.keys.Left.pressed)  { dir.phi   -= rotate_step; changed = true; }
        if (g.keys.Right.pressed) { dir.phi   += rotate_step; changed = true; }
        if (g.keys.Up.pressed)    { dir.theta -= rotate_step; changed = true; }
        if (g.keys.Down.pressed)  { dir.theta += rotate_step; changed = true; }

        // translation
        // Note: The z_over_f factor ensures that a key press yields the same
        // displacement of the object in image pixels, irregardless of how far
        // away the camera is.
        float z_over_f = fabsf(pos.z)/camera_f;
        float x_move_step = (g.resolution.x*0.05f)*z_over_f;
        float y_move_step = (g.resolution.y*0.05f)*z_over_f;
        float z_move_step = 0.1f*fabsf(pos.z);
        if (g.keys.Ctrl.pressed)  { pos.y -= y_move_step; changed = true; }
        if (g.keys.Space.pressed) { pos.y += y_move_step; changed = true; }
        if (g.keys.A.pressed)     { pos.x -= x_move_step; changed = true; }
        if (g.keys.D.pressed)     { pos.x += x_move_step; changed = true; }
        if (g.keys.W.pressed)     { pos.z -= z_move_step; changed = true; }
        if (g.keys.S.pressed)     { pos.z += z_move_step; changed = true; }

        if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
        {
            float3 drag_speeds;
            drag_speeds.x = (g.resolution.x*0.005f)*z_over_f;
            drag_speeds.y = (g.resolution.y*0.005f)*z_over_f;
            drag_speeds.z = 0.01f*fabsf(pos.z);

            if (loc_iView != -1)
            {
                changed |= ImGui::SliderFloat("\xce\xb8##view_dir", &dir.theta, -90.0f, +90.0f, "%.0f deg");
                changed |= ImGui::SliderFloat("\xcf\x86##view_dir", &dir.phi, -180.0f, +180.0f, "%.0f deg");
                changed |= ImGui::DragFloat3("pos##view_pos", &pos.x, &drag_speeds.x);
            }
            if (loc_iCameraF != -1)
                changed |= ImGui::DragFloat("f##camera_f", &camera_f);
            if (loc_iCameraCenter != -1)
                changed |= ImGui::DragFloat2("center##camera_center", &camera_center.x);
        }

        return changed;
    }
    virtual void set_params()
    {
        fraktal_param_2f(loc_iCameraCenter, camera_center.x, camera_center.y);
        fraktal_param_1f(loc_iCameraF, camera_f);
        float iView[4*4];
        float3 r = {
            deg2rad(dir.theta),
            deg2rad(dir.phi),
            0.0f
        };
        compute_view_matrix(iView, pos, r);
        fraktal_param_transpose_matrix4f(loc_iView, iView);
    }
};
