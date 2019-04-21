#pragma once

char *read_file(const char *filename, int *out_size=NULL)
{
    if (!filename)
        return NULL;
    FILE *f = fopen(filename, "rb");
    if (!f)
        return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    char *result = new char[size + 1];
    if (!fread(result, 1, size, f))
        return NULL;
    result[size] = '\0';
    fclose(f);
    if (out_size)
        *out_size = size;
    return result;
}

/*
    Example usage:
        char *data = read_file(filename);
        char *line = read_line(&data);
        while (line) {
            // handle line
            line = read_line(&data);
        }
*/
char *read_line(char **s)
{
    if (!*s) return NULL; // if the user calls read_line(&data) with data=NULL (no data)
    if (!**s) return NULL; // if the user calls read_line(&data) with data[0]=NULL (no line/eof)
    char *begin = *s;
    char *end = *s;
    while (*end && !(*end == '\n' || *end == '\r')) // while !newline
        end++;
    while (*end && (*end == '\n' || *end == '\r')) // while newline
    {
        *end = NULL;
        end++;
    }
    *s = end;
    return begin;
}
