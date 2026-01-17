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

    int ret = fseek(f, 0, SEEK_END);
    assert(ret == 0);
    long size = ftell(f);
    ret = fseek(f, 0, SEEK_SET);
    assert(ret == 0);

    char *bytes = malloc(size);

    size_t in = fread(bytes, 1, size, f);
    assert(in == size);

    fclose(f);

    sn_program_t *prog = NULL;
    sn_error_t status = sn_program_create(&prog, bytes, size);
    if (status != SN_SUCCESS) {
        sn_program_write_error(prog, stderr);
        exit(-1);
    }

    sn_program_run(prog);
    if (prog->status != SN_SUCCESS) {
        sn_program_write_error(prog, stderr);
        exit(-1);
    }

    sn_program_destroy(prog);
    return 0;
}
