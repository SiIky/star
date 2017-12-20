/*
 * <stdio.h>
 *  FILE
 *  fclose()
 *  fopen()
 *  fprintf()
 *  stderr
 *
 * <stdlib.h>
 *  EXIT_FAILURE
 *  EXIT_SUCCESS
 *
 * <string.h>
 *  strcmp()
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "star.h"

#define ifjmp(COND, LBL) if (COND) goto LBL

size_t fsize (FILE * stream)
{
    size_t ret = 0;

    ifjmp(stream == NULL, out);

    long old = ftell(stream);
    ifjmp(old < 0, out);
    ifjmp(fseek(stream, 0, SEEK_END) != 0, out);
    long size = ftell(stream);
    ifjmp(size < 0, out);
    fseek(stream, old, SEEK_SET);

    ret = (size >= 0) ?
        size :
        0 ;

out:
    return ret;
}

void usage (char * cmd)
{
    fprintf(stderr,
            "\t%s c OUTPUT_FILE INPUT_FILES\n"
            "\t%s x INPUT_FILE\n",
            cmd, cmd);
}

void create (int n, char ** args)
{
    FILE * out = NULL;
    FILE * in = NULL;
    struct star_file * star = NULL;

    out = fopen(args[0], "wb");
    ifjmp(out == NULL, out);

    star = star_new(n - 1);
    ifjmp(star == NULL, out);

    for (int i = 1; i < n; i++) {
        fprintf(stderr, "open FILE `%s`\n", args[i]);
        in = fopen(args[i], "rb");
        if (in == NULL)
            fprintf(stderr, "Error opening file `%s`\n", args[i]);
        ifjmp(in == NULL, out);
        size_t size = fsize(in);
        bool res = star_add_file(star, i - 1, args[i], size, in);
        if (!res)
            fprintf(stderr, "Error adding file `%s`", args[i]);
        ifjmp(!res, out);
        fclose(in);
        in = NULL;
        fprintf(stderr, "close FILE `%s`\n", args[i]);
    }

    star_file_offsets(star);

    if (!star_write(star, out))
        fprintf(stderr, "Error writing STAR file `%s`\n",
                *args);

out:
    if (out != NULL)
        fclose(out);

    if (in != NULL)
        fclose(in);

    star_free(star);
}

void extract (int n, char ** args)
{
    ((void)args);
    ((void)n);
}

void list (int n, char ** args)
{
    struct star_file * ret = NULL;
    struct star_file _tmp;

    for (int i = 0; i < n; i++) {
        memset(&_tmp, 0, sizeof(struct star_file));
        FILE * in = fopen(args[i], "rb");

        /* failed to read header or `in` is not a STAR file */
        ifjmp(!star_read_header(&_tmp, in), out);

        {
            ret = malloc(sizeof(struct star_file));
            ifjmp(ret == NULL, out);

            *ret = _tmp;
        }

        /* number of successfully read file headers */
        size_t nfheaders = star_read_fheaders(ret, in);
        ifjmp(ret->header.nfiles != nfheaders, out);

        for (size_t idx = 0; idx < nfheaders; idx++)
            puts((void*) ret->fheaders[idx].path);

out:
        star_free(ret);
    }
}

int main (int argc, char ** argv)
{
    ifjmp(argc < 2, usage);

    void (*todo) (int, char **) = NULL;

    if (strcmp(argv[1], "c") == 0) { /* create */
        ifjmp(argc < 4, usage);
        todo = create;
    } else if (strcmp(argv[1], "x") == 0) { /* extract */
        ifjmp(argc < 3, usage);
        todo = extract;
    } else if (strcmp(argv[1], "l") == 0) { /* list */
        ifjmp(argc < 3, usage);
        todo = list;
    } else {
        goto usage;
    }

    ifjmp(todo == NULL, usage); /* shouldnt happen */

    for (int i = 0; i < argc; i++)
        fprintf(stderr, "%s ", argv[i]);
    putchar('\n');

    todo(argc - 2, argv + 2);

    return EXIT_SUCCESS;

usage:
    usage(*argv);
    return EXIT_FAILURE;
}
