// Developed by Simen Haugo.
// See LICENSE.txt for copyright and licensing details (standard MIT License).

#pragma once
#include <string.h>
#include <stdio.h>
#include <log.h>

static const char *parse_error_start = NULL;
static const char *parse_error_name = NULL;

static void parse_error(const char *at, const char *message)
{
    assert(parse_error_start);
    assert(parse_error_name);
    const char *c = parse_error_start;
    size_t line = 0;
    size_t column = 0;
    while (*c && c <= at)
    {
        if (*c == '\n' || *c == '\r')
        {
            while (*c && (*c == '\n' || *c == '\r'))
                c++;
            line++;
            column = 0;
        }
        else
        {
            c++;
            column++;
        }
    }
    log_err("<%s>:%d:%d: error: %s", parse_error_name, line, column, message);
}

static void parse_alpha(const char **c)
{
    while (**c)
    {
        if ((**c >= 'a' && **c <= 'z') ||
            (**c >= 'A' && **c <= 'Z') ||
            (**c >= '0' && **c <= '9'))
            (*c)++;
        else
            break;
    }
}

static bool parse_is_blank(char c)
{
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

static void parse_blank(const char **c)
{
    while (**c && parse_is_blank(**c))
        (*c)++;
}

static void parse_comment(const char **c)
{
    char c0 = (*c)[0];
    char c1 = (*c)[1];
    if (c0 && c1 && c0 == '/' && c1 == '/')
    {
        while (**c && **c != '\n' && **c != '\r')
            (*c)++;
        while (**c && (**c == '\n' || **c == '\r'))
            (*c)++;
    }
    else if (c0 && c1 && c0 == '/' && c1 == '*')
    {
        while ((*c)[0] && (*c)[1] && !((*c)[0] == '*' && (*c)[1] == '/'))
            (*c)++;
        (*c) += 2;
    }
}

static bool parse_char(const char **c, char match)
{
    if (**c && **c == match)
    {
        *c = *c + 1;
        return true;
    }
    else
        return false;
}

static bool parse_match(const char **c, const char *match)
{
    size_t n = strlen(match);
    if (strncmp(*c, match, n) == 0)
    {
        *c = *c + n;
        return true;
    }
    else
        return false;
}

static bool parse_bool(const char **c, bool *x)
{
    if (parse_match(c, "true"))       *x = true;
    else if (parse_match(c, "True"))  *x = true;
    else if (parse_match(c, "false")) *x = false;
    else if (parse_match(c, "False")) *x = false;
    else                              return false;
    return true;
}

static bool parse_int(const char **c, int *x)
{
    int b;
    int _x;
    if (1 == sscanf(*c, "%d%n", &_x, &b))
    {
        *c = *c + b;
        *x = _x;
        return true;
    }
    return false;
}

static bool parse_float(const char **c, float *x)
{
    int b;
    float _x;
    if (1 == sscanf(*c, "%f%n", &_x, &b))
    {
        *c = *c + b;
        *x = _x;
        return true;
    }
    return false;
}

static bool parse_angle(const char **c, float *x)
{
    int b;
    float _x;
    if (1 == sscanf(*c, "%f%n", &_x, &b))
    {
        *c = *c + b;
        parse_blank(c);
        if (parse_match(c, "deg"))
        {
            *x = _x;
            return true;
        }
        else if (parse_match(c, "rad"))
        {
            *x = _x*(180.0f/3.1415926535897932384626433832795f);
            return true;
        }
        else parse_error(*c, "Error parsing angle: must have either 'deg' or 'rad' as suffix.\n");
    }
    return false;
}

// len: does not include zero-terminator
static bool parse_string(const char **c, const char **v, size_t *len)
{
    char delimiter = '\0';
    if (parse_char(c, '\"'))
        delimiter = '\"';
    else if (parse_char(c, '\''))
        delimiter = '\'';
    else
    {
        parse_error(*c, "Error parsing string: must begin with single or double quotation.\n");
        return false;
    }

    *v = *c;
    while (**c && **c != delimiter)
        c++;
    if (**c == '\0')
    {
        parse_error(*c, "Error parsing string: missing end quotation.\n");
        return false;
    }
    *len = *c - *v;
}

static bool parse_int2(const char **c, int2 *v)
{
    if (!parse_char(c, '('))  { parse_error(*c, "integer tuple must begin with parenthesis.\n"); return false; }
    if (!parse_int(c, &v->x)) { parse_error(*c, "1st tuple component must be an integer.\n"); return false; }
    if (!parse_char(c, ','))  { parse_error(*c, "integer tuple components must be seperated by ','.\n"); return false; }
    if (!parse_int(c, &v->y)) { parse_error(*c, "2nd tuple component must be an integer.\n"); return false; }
    if (!parse_char(c, ')'))  { parse_error(*c, "integer tuple must end with parenthesis.\n"); return false; }
    return true;
}

static bool parse_int3(const char **c, int3 *v)
{
    if (!parse_char(c, '('))  { parse_error(*c, "integer tuple must begin with parenthesis.\n"); return false; }
    if (!parse_int(c, &v->x)) { parse_error(*c, "1st tuple component must be an integer.\n"); return false; }
    if (!parse_char(c, ','))  { parse_error(*c, "tuple components must be seperated by ','.\n"); return false; }
    if (!parse_int(c, &v->y)) { parse_error(*c, "2nd tuple component must be an integer.\n"); return false; }
    if (!parse_char(c, ','))  { parse_error(*c, "tuple components must be seperated by ','.\n"); return false; }
    if (!parse_int(c, &v->z)) { parse_error(*c, "3rd tuple component must be an integer.\n"); return false; }
    if (!parse_char(c, ')'))  { parse_error(*c, "integer tuple must end with parenthesis.\n"); return false; }
    return true;
}

static bool parse_int4(const char **c, int4 *v)
{
    if (!parse_char(c, '('))  { parse_error(*c, "integer tuple must begin with parenthesis.\n"); return false; }
    if (!parse_int(c, &v->x)) { parse_error(*c, "1st tuple component must be an integer.\n"); return false; }
    if (!parse_char(c, ','))  { parse_error(*c, "tuple components must be seperated by ','.\n"); return false; }
    if (!parse_int(c, &v->y)) { parse_error(*c, "2nd tuple component must be an integer.\n"); return false; }
    if (!parse_char(c, ','))  { parse_error(*c, "tuple components must be seperated by ','.\n"); return false; }
    if (!parse_int(c, &v->z)) { parse_error(*c, "3rd tuple component must be an integer.\n"); return false; }
    if (!parse_char(c, ','))  { parse_error(*c, "tuple components must be seperated by ','.\n"); return false; }
    if (!parse_int(c, &v->w)) { parse_error(*c, "4th tuple component must be an integer.\n"); return false; }
    if (!parse_char(c, ')'))  { parse_error(*c, "integer tuple must end with parenthesis.\n"); return false; }
    return true;
}

static bool parse_angle2(const char **c, angle2 *v)
{
    if (!parse_char(c, '('))        { parse_error(*c, "angle tuple must begin with parenthesis.\n"); return false; }
    if (!parse_angle(c, &v->theta)) { parse_error(*c, "1st tuple component must be a number.\n"); return false; }
    if (!parse_char(c, ','))        { parse_error(*c, "tuple components must be seperated by ','.\n"); return false; }
    if (!parse_angle(c, &v->phi))   { parse_error(*c, "2nd tuple component must be a number.\n"); return false; }
    if (!parse_char(c, ')'))        { parse_error(*c, "angle tuple must end with parenthesis.\n"); return false; }
    return true;
}

static bool parse_float2(const char **c, float2 *v)
{
    if (!parse_char(c, '('))    { parse_error(*c, "tuple must begin with parenthesis.\n"); return false; }
    if (!parse_float(c, &v->x)) { parse_error(*c, "1st tuple component must be a number.\n"); return false; }
    if (!parse_char(c, ','))    { parse_error(*c, "tuple components must be seperated by ','.\n"); return false; }
    if (!parse_float(c, &v->y)) { parse_error(*c, "2nd tuple component must be a number.\n"); return false; }
    if (!parse_char(c, ')'))    { parse_error(*c, "tuple must end with parenthesis.\n"); return false; }
    return true;
}

static bool parse_float3(const char **c, float3 *v)
{
    if (!parse_char(c, '('))    { parse_error(*c, "tuple must begin with parenthesis.\n"); return false; }
    if (!parse_float(c, &v->x)) { parse_error(*c, "1st tuple component must be a number.\n"); return false; }
    if (!parse_char(c, ','))    { parse_error(*c, "tuple components must be seperated by ','.\n"); return false; }
    if (!parse_float(c, &v->y)) { parse_error(*c, "2nd tuple component must be a number.\n"); return false; }
    if (!parse_char(c, ','))    { parse_error(*c, "tuple components must be seperated by ','.\n"); return false; }
    if (!parse_float(c, &v->z)) { parse_error(*c, "3rd tuple component must be a number.\n"); return false; }
    if (!parse_char(c, ')'))    { parse_error(*c, "tuple must end with parenthesis.\n"); return false; }
    return true;
}

static bool parse_float4(const char **c, float4 *v)
{
    if (!parse_char(c, '('))    { parse_error(*c, "tuple must begin with parenthesis.\n"); return false; }
    if (!parse_float(c, &v->x)) { parse_error(*c, "1st tuple component must be a number.\n"); return false; }
    if (!parse_char(c, ','))    { parse_error(*c, "tuple components must be seperated by ','.\n"); return false; }
    if (!parse_float(c, &v->y)) { parse_error(*c, "2nd tuple component must be a number.\n"); return false; }
    if (!parse_char(c, ','))    { parse_error(*c, "tuple components must be seperated by ','.\n"); return false; }
    if (!parse_float(c, &v->z)) { parse_error(*c, "3rd tuple component must be a number.\n"); return false; }
    if (!parse_char(c, ','))    { parse_error(*c, "tuple components must be seperated by ','.\n"); return false; }
    if (!parse_float(c, &v->w)) { parse_error(*c, "4th tuple component must be a number.\n"); return false; }
    if (!parse_char(c, ')'))    { parse_error(*c, "tuple must end with parenthesis.\n"); return false; }
    return true;
}

static bool parse_inside_list = false;
static bool parse_list_first = false;
static bool parse_list_error = false;

static bool parse_begin_list(const char **c)
{
    if (**c == '(')
    {
        *c = *c + 1;
        parse_inside_list = true;
        parse_list_first = true;
        parse_list_error = false;
        return true;
    }
    return false;
}

static bool parse_next_in_list(const char **c)
{
    assert(parse_inside_list);
    if (parse_list_error)
    {
        parse_error(*c, "unexpected argument.\n");
        return false;
    }
    parse_blank(c);
    if (parse_char(c, ')'))
        return false;
    if (!parse_list_first)
        if (!parse_char(c, ',')) { parse_error(*c, "arguments must be seperated by ','.\n"); return false; }
    parse_blank(c);
    parse_list_first = false;
    return true;
}

static bool parse_end_list(const char **c)
{
    assert(parse_inside_list);
    parse_inside_list = false;
    if (parse_list_error)
        return false;
    return true;
}

static void parse_list_unexpected()
{
    parse_list_error = true;
}

#define declare_parse_argument_(type) \
    static bool parse_argument_##type(const char **c, const char *name, type *v) \
    { \
        if (parse_match(c, name)) \
        { \
            parse_blank(c); \
            if (!parse_char(c, '=')) { parse_error(*c, "expected '=' between argument name and value.\n"); return false; } \
            parse_blank(c); \
            if (!parse_##type(c, v)) { parse_error(*c, "unexpected expression after '='.\n"); return false; } \
            return true; \
        } \
        return false; \
    }

declare_parse_argument_(bool);
declare_parse_argument_(int);
declare_parse_argument_(int2);
declare_parse_argument_(int3);
declare_parse_argument_(int4);
declare_parse_argument_(float);
declare_parse_argument_(angle);
declare_parse_argument_(angle2);
declare_parse_argument_(float2);
declare_parse_argument_(float3);
declare_parse_argument_(float4);

#if 0
static bool parse_argument_string(const char **c, const char *name, const char **v)
{
    if (parse_match(c, name))
    {
        parse_blank(c);
        if (!parse_char(c, '=')) { parse_error(*c, "Error parsing argument: expected '=' between identifier and value.\n"); return false; }
        if (!parse_string(c, v)) { parse_error(*c, "Error parsing argument value: unexpected type after '='.\n"); return false; }
    }
    return false;
}
#endif

static bool parse_param_meta(const char **c, fParams *p, int param)
{
    fParamType type = p->type[param];
    while (parse_next_in_list(c)) {
        if (type == FRAKTAL_PARAM_FLOAT ||
            type == FRAKTAL_PARAM_INT)
        {
            if (parse_argument_float(c, "mean", (float*)&p->mean[param])) continue;
            else if (parse_argument_float(c, "scale", (float*)&p->scale[param])) continue;
        }

        if (type == FRAKTAL_PARAM_FLOAT_VEC2 ||
            type == FRAKTAL_PARAM_INT_VEC2)
        {
            if (parse_argument_float2(c, "mean", (float2*)&p->mean[param])) continue;
            else if (parse_argument_float2(c, "scale", (float2*)&p->scale[param])) continue;
        }

        if (type == FRAKTAL_PARAM_FLOAT_VEC3 ||
            type == FRAKTAL_PARAM_INT_VEC3)
        {
            if (parse_argument_float3(c, "mean", (float3*)&p->mean[param])) continue;
            else if (parse_argument_float3(c, "scale", (float3*)&p->scale[param])) continue;
        }

        if (type == FRAKTAL_PARAM_FLOAT_VEC4 ||
            type == FRAKTAL_PARAM_INT_VEC4)
        {
            if (parse_argument_float4(c, "mean", (float4*)&p->mean[param])) continue;
            else if (parse_argument_float4(c, "scale", (float4*)&p->scale[param])) continue;
        }

        parse_list_unexpected();
    }

    if (!parse_end_list(c))
    {
        parse_error(*c, "invalid parameter meta arguments.\n");
        return false;
    }
    return true;
}

static bool parse_param(const char **c, fParams *p, int param)
{
    if (param >= FRAKTAL_MAX_PARAMS)
    {
        parse_error(*c, "exceeded maximum number of parameters in kernel.\n");
        return false;
    }

    // Get type
    {
        fParamType type;
        parse_blank(c);
        if      (parse_match(c, "float"))     { type = FRAKTAL_PARAM_FLOAT; }
        else if (parse_match(c, "vec2"))      { type = FRAKTAL_PARAM_FLOAT_VEC2; }
        else if (parse_match(c, "vec3"))      { type = FRAKTAL_PARAM_FLOAT_VEC3; }
        else if (parse_match(c, "vec4"))      { type = FRAKTAL_PARAM_FLOAT_VEC4; }
        else if (parse_match(c, "mat2"))      { type = FRAKTAL_PARAM_FLOAT_MAT2; }
        else if (parse_match(c, "mat3"))      { type = FRAKTAL_PARAM_FLOAT_MAT3; }
        else if (parse_match(c, "mat4"))      { type = FRAKTAL_PARAM_FLOAT_MAT4; }
        else if (parse_match(c, "int"))       { type = FRAKTAL_PARAM_INT; }
        else if (parse_match(c, "ivec2"))     { type = FRAKTAL_PARAM_INT_VEC2; }
        else if (parse_match(c, "ivec3"))     { type = FRAKTAL_PARAM_INT_VEC3; }
        else if (parse_match(c, "ivec4"))     { type = FRAKTAL_PARAM_INT_VEC4; }
        else if (parse_match(c, "sampler1D")) { type = FRAKTAL_PARAM_SAMPLER1D; p->sampler_count++; }
        else if (parse_match(c, "sampler2D")) { type = FRAKTAL_PARAM_SAMPLER2D; p->sampler_count++; }
        else
        {
            parse_error(*c, "invalid parameter type.\n");
            return false;
        }
        p->type[param] = type;
    }

    // Get name
    {
        const char *name_start;
        size_t name_len;
        parse_blank(c);
        name_start = *c;
        parse_alpha(c);
        const char *name_end = *c;
        if (name_start == name_end)
        {
            parse_error(*c, "missing parameter name\n");
            return false;
        }
        if (**c == '\0')
        {
            parse_error(name_start, "file ends prematurely after this parameter.\n");
            return false;
        }
        name_len = name_end - name_start;
        if (name_len > FRAKTAL_MAX_PARAM_NAME_LEN)
        {
            parse_error(*c, "parameter name is too long.\n");
            return false;
        }
        memcpy(p->name[param], name_start, name_len);
        p->name[param][name_len] = '\0';
    }

    // Get meta
    parse_blank(c);
    if (parse_begin_list(c))
    {
        return parse_param_meta(c, p, param);
    }
    else if (parse_char(c, ';'))
    {
        p->mean[param].x = 0.0f;
        p->mean[param].y = 0.0f;
        p->mean[param].z = 0.0f;
        p->mean[param].w = 0.0f;
        p->scale[param].x = 1.0f;
        p->scale[param].y = 1.0f;
        p->scale[param].z = 1.0f;
        p->scale[param].w = 1.0f;
        return true;
    }
    else
    {
        parse_error(*c, "unexpected symbol after parameter name.\n");
        return false;
    }
}

static bool parse_fraktal_source(char *fs, fParams *p, const char *name)
{
    parse_error_start = fs;
    parse_error_name = name;
    char *cw = fs;
    while (*cw)
    {
        const char **c = (const char**)&cw;
        parse_comment(c);
        parse_blank(c);
        if (parse_match(c, "uniform"))
        {
            int param = p->count;
            if (!parse_param(c, p, param))
                return false;
            p->count++;
        }
        cw++;
    }
    return true;
}
