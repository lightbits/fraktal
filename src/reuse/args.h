// Changelog:
// 9. april 2019: No longer using strdup (availability issues across compilers).
// 7. april 2019: Added arg_bool.
// 7. april 2019: Added arg_get_exe_dir. Removed flags. Use arg_err in arg_help prompt.

// Example usage:
// int main(int argc, char **argv)
// {
//     arg_int32(&dpi,     100,  "-dpi",    "DPI resolution");
//     arg_string(&eqname, "eq", "-out",    "Base filename of output PNGs");
//     arg_int32(&ptsize,  12,   "-ptsize", "Font size");
//     arg_string(&input,  NULL, "-i",      "Input filename");
//     if (!arg_parse(argc, argv))
//     {
//         arg_help();
//         return 1;
//     }
//     return 0;
// }
#pragma once
#include <stdint.h>
void arg_int32  (int32_t *x,  int32_t x0,  const char *key, const char *msg);
void arg_uint32 (uint32_t *x, uint32_t x0, const char *key, const char *msg);
void arg_float32(float *x,    float x0,    const char *key, const char *msg);
void arg_string (const char **x, const char *x0, const char *key, const char *msg);
void arg_bool   (bool *x, bool x0, const char *key, const char *msg);
const char *arg_get_exe_dir();
bool arg_parse  (int argc, char **argv);
void arg_help   ();

//
// implementation follows
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#ifndef arg_err
#define arg_err(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__)
#endif
#ifndef arg_assert
#include <assert.h>
#define arg_assert assert
#endif

typedef int arg_type_t;
enum arg_type_
{
    arg_type_int32 = 0,
    arg_type_uint32,
    arg_type_float32,
    arg_type_bool,
    arg_type_string
};

enum { MAX_ARGS = 1024 };

struct arg_t
{
    arg_type_t type;
    void *ptr;
    const char *msg;
    const char *key; // note: this should be freed, we currently don't
};

struct args_t
{
    int num_expected;
    const char *exe_dir;
    arg_t args[MAX_ARGS];
};

static args_t global_args = {0};

#define arg_routine_macro(SUFFIX, TYPENAME) \
    void arg_##SUFFIX(TYPENAME *x, TYPENAME x0, const char *key, const char *msg) \
    { \
        *x = x0; \
        arg_t arg = {0}; \
        arg.type = arg_type_##SUFFIX; \
        arg.ptr = x; \
        arg.key = key; \
        arg.msg = msg; \
        assert(global_args.num_expected < MAX_ARGS); \
        global_args.args[global_args.num_expected++] = arg; \
    }

arg_routine_macro(int32, int32_t);
arg_routine_macro(uint32, uint32_t);
arg_routine_macro(float32, float);
arg_routine_macro(bool, bool);
arg_routine_macro(string, const char*);

bool arg_parse(int argc, char **argv)
{
    if (global_args.num_expected == 0)
        return true;

    for (int i = 0; i < argc; i++)
    {
        bool match = false;
        size_t len_arg = strlen(argv[i]);
        for (int j = 0; j < global_args.num_expected; j++)
        {
            arg_t *arg = &global_args.args[j];
            size_t len_key = strlen(arg->key);
            if (strncmp(argv[i], arg->key, len_key) == 0)
            {
                match = true;

                if (arg->type == arg_type_bool)
                {
                    *(bool*)arg->ptr = true;
                    break;
                }

                char *value = NULL;
                if (len_arg > len_key)
                {
                    value = argv[i] + len_key;
                    if (*value == '=')
                        value++;
                }
                else if (i + 1 < argc)
                {
                    value = argv[++i];
                }
                else
                {
                    arg_err("Missing value for argument '%s'.\n", argv[i]);
                    return false;
                }

                if      (arg->type == arg_type_int32)   { if (1 != sscanf(value, "%d", (int32_t*)arg->ptr))  { arg_err("'%s' option expects an integer, got '%s'.\n", arg->key, value); return false; } }
                else if (arg->type == arg_type_uint32)  { if (1 != sscanf(value, "%u", (uint32_t*)arg->ptr)) { arg_err("'%s' option expects an integer, got '%s'.\n", arg->key, value); return false; } }
                else if (arg->type == arg_type_float32) { if (1 != sscanf(value, "%f", (float*)arg->ptr))    { arg_err("'%s' option expects a float, got '%s'.\n",  arg->key, value); return false; } }
                else if (arg->type == arg_type_string)  *(const char**)arg->ptr = value;
                else arg_assert(false && "Undefined argument type");

                break;
            }
        }
        if (!match)
        {
            if (i == 0)
            {
                char *exe_dir = (char*)malloc(strlen(argv[i]) + 1);
                arg_assert(exe_dir);
                strcpy(exe_dir, argv[i]);
                for (size_t j = 0; j < strlen(exe_dir); j++)
                {
                    #ifdef _WIN32
                    if (exe_dir[j] == '/') exe_dir[j] = '\\';
                    #else
                    if (exe_dir[j] == '\\') exe_dir[j] = '/';
                    #endif
                }
                char *unix = strrchr(exe_dir, '/');
                char *windows = strrchr(exe_dir, '\\');
                char *last = unix > windows ? unix : windows;
                if (last)
                {
                    last[1] = '\0';
                    global_args.exe_dir = exe_dir;
                }
                else
                {
                    free(exe_dir);
                }
            }
            else
            {
                arg_err("Unrecognized argument '%s'.\n", argv[i]);
                return false;
            }
        }
    }

    if (!global_args.exe_dir)
    {
        #ifdef _WIN32
        global_args.exe_dir = ".\\";
        #else
        global_args.exe_dir = "./";
        #endif
    }

    return true;
}

const char *arg_get_exe_dir()
{
    return global_args.exe_dir;
}

void arg_help()
{
    for (int i = 0; i < global_args.num_expected; i++)
    {
        arg_t *arg = &global_args.args[i];
        size_t len = strlen(arg->key);
        arg_err("  %s ", arg->key);
        for (int j = len; j < 8; j++)
            arg_err("%s", ".");
        arg_err("... %s ", arg->msg);
        if      (arg->type == arg_type_int32)   arg_err("[%d]\n", *(int32_t*)arg->ptr);
        else if (arg->type == arg_type_uint32)  arg_err("[%u]\n", *(uint32_t*)arg->ptr);
        else if (arg->type == arg_type_float32) arg_err("[%f]\n", *(float*)arg->ptr);
        else if (arg->type == arg_type_string)  arg_err("[%s]\n", *(const char**)arg->ptr);
        else if (arg->type == arg_type_bool)    arg_err("[%s]\n", *(const char**)arg->ptr ? "true" : "false");
        else arg_assert(false && "Undefined argument type");
    }
}
