#pragma once
#include "log.h"
#define arg_err log_err
#include <args.h>

bool test_cli_parser()
{
    char *argv[] = {
        "/bin/usr/fraktal/fraktal",
        "-i",
        ".\\data\\default_model.glsl",
        "-nogui",
        "-framerate",
        "12",
        "-vseconds",
        "12",
        "-ovideo%04d.png"
    };
    int argc = sizeof(argv)/sizeof(char*);

    const char *input;
    const char *output;
    const char *renderer;
    int framerate;
    int vseconds;
    int vframes;
    int start_frame;
    int start_seconds;
    int end_frame;
    int end_seconds;
    int samples;
    bool nogui;
    bool denoise;
    arg_string(&input,        "./data/default_model.glsl", "-i", "Input file");
    arg_string(&output,       "output",   "-o",             "Output file");
    arg_string(&renderer,     "diffuse",  "-renderer",      "'fast', 'diffuse', 'simplified-specular', 'specular'");
    arg_int32(&framerate,     0,          "-framerate",     "Frames per second of video");
    arg_int32(&vseconds,      0,          "-vseconds",      "Video duration (seconds)");
    arg_int32(&vframes,       0,          "-vframes",       "Video duration (frames)");
    arg_int32(&start_frame,   0,          "-start_frame",   "Video start time (frames)");
    arg_int32(&start_seconds, 0,          "-start_seconds", "Video start time (seconds)");
    arg_int32(&end_frame,     0,          "-end_frame",     "Video end time (frames)");
    arg_int32(&end_seconds,   0,          "-end_seconds",   "Video end time (seconds)");
    arg_int32(&samples,       16,          "-spp",           "Samples per pixel (has no effect when using 'fast' renderer)");
    arg_bool(&nogui,          false,      "-nogui",         "Disables GUI preview while rendering output");
    arg_bool(&denoise,        true,       "-denoise",       "Enables de-noising algorithm (suitable for smooth scenes)");

    if (!arg_parse(argc, argv))
    {
        arg_help();
        return false;
    }
    return true;
}
