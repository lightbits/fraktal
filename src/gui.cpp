/*
    Stand-alone GUI application for fraktal.
    Developed by Simen Haugo.
    See LICENSE.txt for copyright and licensing details (standard MIT License).
*/
#define FRAKTAL_GUI
#include "fraktal.cpp"
#include <args.h>
int main(int argc, char **argv)
{
    int width,height;
    const char *model;
    const char *color;
    const char *compose;
    const char *geometry;
    arg_int32(&width,     200,                               "-width",    "Render resolution (x)");
    arg_int32(&height,    200,                               "-height",   "Render resolution (y)");
    arg_string(&model,    "./examples/vase.f",               "-model",    "Path to a .f kernel containing model definition");
    arg_string(&color,    "./libf/publication.f",            "-color",    "Path to a .f kernel containing color renderer definition");
    arg_string(&compose,  "./libf/mean_and_gamma_correct.f", "-compose",  "Path to a .f kernel containing color composer definition");
    arg_string(&geometry, "./libf/geometry.f",               "-geometry", "Path to a .f kernel containing geometry renderer definition");
    if (!arg_parse(argc, argv))
    {
        arg_help();
        return 1;
    }
    fraktal_create_context();
    fg_configure(model, color, compose, geometry, width, height);
    fg_show();
    return 0;
}
