#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "snscript.h"

void write_error(const char *file, sn_error_t status, sn_program_t *prog)
{
    int line = 0;
    int col = 0;
    const char *str = NULL;

    sn_program_error_pos(prog, &line, &col);
    sn_program_error_symbol(prog, &str);

    fprintf(stderr,
            "%s:%d:%d %s%s%s\n",
            file,
            line,
            col,
            sn_error_str(status),
            str == NULL ? "" : ": ",
            str == NULL ? "" : str);
}

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
        write_error(argv[1], status, prog);
        exit(-1);
    }

    status = sn_program_build(prog);
    if (status != SN_SUCCESS) {
        write_error(argv[1], status, prog);
        exit(-1);
    }

    sn_value_t *arg = sn_value_create();
    if (argc == 3) {
        sn_value_set_integer(arg, atoi(argv[2]));
    }
    else {
        sn_value_set_integer(arg, 0);
    }

    sn_value_t *value = sn_value_create();
    status = sn_program_run_main(prog, arg, value);
    if (status != SN_SUCCESS) {
        write_error(argv[1], status, prog);
        exit(-1);
    }
    sn_value_destroy(value);
    sn_program_destroy(prog);
    return 0;
}
