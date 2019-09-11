#pragma once

struct Widget
{
    virtual void default_values(guiState &g) = 0;
    virtual void deserialize(const char **cc) = 0;
    virtual void serialize(FILE *f) = 0;
    virtual void get_param_offsets(fKernel *f) = 0;
    virtual bool is_active() = 0;
    virtual bool update(guiState g) = 0;
    virtual void set_params() = 0;
};
