#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "snprogram.h"

int main(int argc, char **argv)
{
    if (argc < 2) {
        exit(-1);
    }

    FILE *f = fopen(argv[1], "rb");
    assert(f);

    int status = fseek(f, 0, SEEK_END);
    assert(status == 0);
    long size = ftell(f);
    status = fseek(f, 0, SEEK_SET);
    assert(status == 0);

    char *bytes = malloc(size);

    size_t in = fread(bytes, 1, size, f);
    assert(in == size);

    fclose(f);

    sn_program_t *prog = sn_program_create(bytes, size);
    if (prog->status != SN_SUCCESS) {
        sn_program_write_error(prog, stderr);
        exit(-1);
    }

    sn_program_run(prog);
    sn_program_destroy(prog);
    return 0;
}
