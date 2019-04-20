// Developed by Simen Haugo.
// See LICENSE.txt for copyright and licensing details (standard MIT License).

#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

static struct logfile_t
{
    size_t bytes;
    size_t capacity;
    char *begin;
    char *str;
} logfile;

static void log_init()
{
    logfile.capacity = 1024*1024;
    logfile.begin = (char*)malloc(logfile.capacity*sizeof(char));
    logfile.str = logfile.begin;
    logfile.str[0] = '\0';
    assert(logfile.begin);
}

static void log_clear()
{
    if (!logfile.begin)
        log_init();
    assert(logfile.begin);
    logfile.str = logfile.begin;
    logfile.str[0] = '\0';
    logfile.bytes = 0;
}

static void log_err(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int bytes = vfprintf(stderr, fmt, args);
    va_end(args);

    if (!logfile.begin)
        log_init();

    assert(logfile.begin);
    assert(logfile.str);
    assert(logfile.capacity > 0);
    assert(logfile.bytes <= logfile.capacity);

    if (logfile.bytes + bytes < logfile.capacity)
    {
        va_start(args, fmt);
        logfile.str += vsprintf(logfile.str, fmt, args);
        va_end(args);
        logfile.bytes += bytes;
    }
}

static const char *log_get_buffer()
{
    if (!logfile.begin)
        log_init();
    return logfile.begin;
}
