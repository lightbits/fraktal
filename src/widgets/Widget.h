#pragma once

struct Widget
{
    virtual void get_param_offsets() = 0;
    virtual bool update() = 0;
    virtual void set_params() = 0;
};
