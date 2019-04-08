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
