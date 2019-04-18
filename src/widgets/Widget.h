#pragma once

struct Widget
{
    virtual void get_param_offsets(fKernel *f) = 0;
    virtual bool update(guiState g) = 0;
    virtual void set_params() = 0;
};
