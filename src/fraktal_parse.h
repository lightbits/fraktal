// Developed by Simen Haugo.
// See LICENSE.txt for copyright and licensing details (standard MIT License).

#pragma once
#include <string.h>
#include <stdio.h>
#include "fraktal_types.h"
#include "log.h"

void parse_alpha(const char **c)
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

bool parse_is_blank(char c)
{
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

void parse_blank(const char **c)
{
    while (**c && parse_is_blank(**c))
        (*c)++;
}

void parse_comment(const char **c)
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

bool parse_char(const char **c, char match)
{
    if (**c && **c == match)
    {
        *c = *c + 1;
        return true;
    }
    else
        return false;
}

bool parse_match(const char **c, const char *match)
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

bool parse_int(const char **c, int *x)
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

bool parse_float(const char **c, float *x)
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

bool parse_angle(const char **c, float *x)
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
        else log_err("Error parsing angle: must have either 'deg' or 'rad' as suffix.\n");
    }
    return false;
}

bool parse_int2(const char **c, int2 *v)
{
    if (!parse_char(c, '('))  { log_err("Error parsing integer tuple: must begin with parenthesis.\n"); return false; }
    if (!parse_int(c, &v->x)) { log_err("Error parsing integer tuple: first component must be an integer.\n"); return false; }
    if (!parse_char(c, ','))  { log_err("Error parsing integer tuple: components must be seperated by ','.\n"); return false; }
    if (!parse_int(c, &v->y)) { log_err("Error parsing integer tuple: second component must be an integer.\n"); return false; }
    if (!parse_char(c, ')'))  { log_err("Error parsing integer tuple: must end with parenthesis.\n"); return false; }
    return true;
}

bool parse_angle2(const char **c, angle2 *v)
{
    if (!parse_char(c, '('))        { log_err("Error parsing angle tuple: must begin with parenthesis.\n"); return false; }
    if (!parse_angle(c, &v->theta)) { log_err("Error parsing angle tuple: first component must be a number.\n"); return false; }
    if (!parse_char(c, ','))        { log_err("Error parsing angle tuple: components must be seperated by ','.\n"); return false; }
    if (!parse_angle(c, &v->phi))   { log_err("Error parsing angle tuple: second component must be a number.\n"); return false; }
    if (!parse_char(c, ')'))        { log_err("Error parsing angle tuple: must end with parenthesis.\n"); return false; }
    return true;
}

bool parse_float2(const char **c, float2 *v)
{
    if (!parse_char(c, '('))    { log_err("Error parsing tuple: must begin with parenthesis.\n"); return false; }
    if (!parse_float(c, &v->x)) { log_err("Error parsing tuple: first component must be a number.\n"); return false; }
    if (!parse_char(c, ','))    { log_err("Error parsing tuple: components must be seperated by ','.\n"); return false; }
    if (!parse_float(c, &v->y)) { log_err("Error parsing tuple: second component must be a number.\n"); return false; }
    if (!parse_char(c, ')'))    { log_err("Error parsing tuple: must end with parenthesis.\n"); return false; }
    return true;
}

bool parse_float3(const char **c, float3 *v)
{
    if (!parse_char(c, '('))   { log_err("Error parsing tuple: must begin with parenthesis.\n"); return false; }
    if (!parse_float(c, &v->x)) { log_err("Error parsing tuple: first component must be a number.\n"); return false; }
    if (!parse_char(c, ','))   { log_err("Error parsing tuple: components must be seperated by ','.\n"); return false; }
    if (!parse_float(c, &v->y)) { log_err("Error parsing tuple: second component must be a number.\n"); return false; }
    if (!parse_char(c, ','))   { log_err("Error parsing tuple: components must be seperated by ','.\n"); return false; }
    if (!parse_float(c, &v->z)) { log_err("Error parsing tuple: third component must be a number.\n"); return false; }
    if (!parse_char(c, ')'))   { log_err("Error parsing tuple: must end with parenthesis.\n"); return false; }
    return true;
}

static bool parse_inside_list = false;
static bool parse_list_first = false;
static bool parse_list_error = false;

bool parse_begin_list(const char **c)
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

bool parse_next_in_list(const char **c)
{
    assert(parse_inside_list);
    if (parse_list_error)
        return false;
    parse_blank(c);
    if (parse_char(c, ')'))
        return false;
    if (!parse_list_first)
        if (!parse_char(c, ',')) { log_err("Error parsing list: components must be seperated by ','.\n"); return false; }
    parse_blank(c);
    parse_list_first = false;
    return true;
}

bool parse_end_list(const char **c)
{
    assert(parse_inside_list);
    parse_inside_list = false;
    if (parse_list_error)
        return false;
    return true;
}

void parse_list_unexpected()
{
    log_err("Error parsing list of arguments: unexpected argument.\n");
    parse_list_error = true;
}

#define declare_parse_argument_(type) \
    bool parse_argument_##type(const char **c, const char *name, type *v) \
    { \
        if (parse_match(c, name)) \
        { \
            parse_blank(c); \
            if (!parse_char(c, '=')) { log_err("Error parsing argument: expected '=' between identifier and value.\n"); return false; } \
            if (!parse_##type(c, v)) { log_err("Error parsing argument value: unexpected type after '='.\n"); return false; } \
            return true; \
        } \
        return false; \
    }

declare_parse_argument_(int);
declare_parse_argument_(int2);
declare_parse_argument_(float);
declare_parse_argument_(angle);
declare_parse_argument_(angle2);
declare_parse_argument_(float2);
declare_parse_argument_(float3);
