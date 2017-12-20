/*
 * <errno.h>
 *  errno
 *  strerror()
 *
 * <stdio.h>
 *  FILE
 *  fclose()
 *  fopen()
 *  fprintf()
 *  printf()
 *  stderr
 *
 * <stdlib.h>
 *  EXIT_FAILURE
 *  EXIT_SUCCESS
 *  qsort()
 *
 * <string.h>
 *  strcmp()
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "star.h"

#define ifjmp(COND, LBL) if (COND) goto LBL

#define eprintf(MSG, ...)   (fprintf(stderr, MSG "\n", __VA_ARGS__))
#define errprintf(MSG, ...) (fprintf(stderr, MSG ": %s\n", __VA_ARGS__, strerror(errno)))

u64 fsize (FILE * stream)
{
    u64 ret = 0;

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
            "%s c OUTPUT_FILE INPUT_FILES\n"
            "\tCreate a STAR file named OUTPUT_FILE with INPUT_FILES.\n"
            "%s x INPUT_FILES\n"
            "\tExtract INPUT_FILES.\n"
            "%s l INPUT_FILES\n"
            "\tList files in INPUT_FILES.\n",
            cmd, cmd, cmd);
}

void create (int n, char ** args)
{
    FILE * out = NULL;
    FILE * in = NULL;
    struct star_file * star = NULL;

    star = star_new(n - 1);
    ifjmp(star == NULL, out);

    qsort(args + 1, n - 1, sizeof(char *), star_strcmp);

    for (int i = 1; i < n; i++) {
        in = fopen(args[i], "rb");
        if (in == NULL)
            errprintf("Error opening `%s`", args[i]);
        ifjmp(in == NULL, out);

        u64 size = fsize(in);
        printf("Archiving `%s`\n", args[i]);
        bool res = star_add_file(star, i - 1,
                                 (void*) args[i], size, in);
        if (!res)
            errprintf("Error adding file `%s`", args[i]);
        ifjmp(!res, out);
        in = (fclose(in), NULL);
    }

    star_file_offsets(star);

    out = fopen(args[0], "wb");
    if (out == NULL)
        errprintf("Error opening `%s`", args[0]);
    ifjmp(out == NULL, out);

    if (!star_write(star, out))
        eprintf("Error writing STAR file `%s`", *args);

out:
    if (out != NULL)
        fclose(out);

    if (in != NULL)
        fclose(in);

    star_free(star);
}

/* TODO: (maybe?) create file hierarchy (directories) */
void extract (int n, char ** args)
{
    for (int i = 0; i < n; i++) {
        FILE * in = fopen(args[i], "rb");
        if (in == NULL) {
            errprintf("Could not open `%s`", args[i]);
            continue;
        }

        struct star_file * star = star_read(in);
        fclose(in);
        if (star == NULL) {
            eprintf("An error occurred reading `%s`", args[i]);
            continue;
        }

        for (u64 fi = 0; fi < star->header.nfiles; fi++) {
            FILE * out = fopen((void*) star->fheaders[fi].path, "w+b");
            if (out == NULL) {
                errprintf("Could not open `%s`",
                          star->fheaders[fi].path);
                continue;
            }

            printf("Extracting `%s`\n", star->fheaders[fi].path);
            u64 w = fwrite(star->fdata[fi],
                           star->fheaders[fi].size, 1, out);
            if (w != 1)
                eprintf("An error occurred writing `%s`",
                        star->fheaders[fi].path);

            fclose(out);
        }

        star_free(star);
    }
}

/* TODO: read headers only, not fdata */
void list (int n, char ** args)
{
    for (int i = 0; i < n; i++) {
        FILE * in = fopen(args[i], "rb");
        if (in == NULL) {
            errprintf("Could not open `%s`", args[i]);
            continue;
        }

        struct star_file * star = star_read(in);
        fclose(in);

        if (star == NULL) {
            eprintf("An error occurred reading `%s`", args[i]);
            continue;
        }

        printf("%s:\n", args[i]);
        for (u64 idx = 0; idx < star->header.nfiles; idx++)
            printf("\t%s\n", star->fheaders[idx].path);

        star_free(star);
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
    }

    ifjmp(todo == NULL, usage);

    todo(argc - 2, argv + 2);

    return EXIT_SUCCESS;

usage:
    usage(*argv);
    return EXIT_FAILURE;
}
