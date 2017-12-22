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
    eprintf(
            "%s c ARCHIVE FILE...\n"
            "\tCreate a STAR named ARCHIVE with FILE.\n"
            "%s x ARCHIVE [FILE]...\n"
            "\tIf no FILE is given, extract every file of ARCHIVE. Else extract only FILE from ARCHIVE.\n"
            "%s l ARCHIVE...\n"
            "\tList files in ARCHIVE.\n",
            cmd, cmd, cmd);
}

/* TODO: better file names handling */
void create (int n, char ** args)
{
    FILE * out = NULL;
    FILE * in = NULL;
    struct star_file * star = NULL;

    star = star_new(n - 1);
    ifjmp(star == NULL, out);

    for (int i = 1; i < n; i++) {
        in = fopen(args[i], "rb");
        if (in == NULL)
            errprintf("Error opening `%s`", args[i]);
        ifjmp(in == NULL, out);

        u64 size = fsize(in);
        eprintf("Archiving `%s`", args[i]);
        bool res = star_add_file(star, i - 1,
                                 (void *) args[i], size, in);
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

#define _extract_file_id(S, ID) do {                              \
    FILE * out = fopen((void *) (S)->fheaders[(ID)].path, "w+b"); \
    if (out == NULL) {                                            \
        errprintf("Could not open `%s`",                          \
                  (S)->fheaders[(ID)].path);                      \
        break;                                                    \
    }                                                             \
                                                                  \
    eprintf("Extracting `%s`", (S)->fheaders[(ID)].path);         \
    u64 w = fwrite((S)->fdata[(ID)],                              \
                   (S)->fheaders[(ID)].size, 1, out);             \
    if (w != 1)                                                   \
        eprintf("An error occurred writing `%s`",                 \
                (S)->fheaders[(ID)].path);                        \
                                                                  \
    fclose(out);                                                  \
} while (0)

/* TODO: (maybe?) create file hierarchy (directories) */
void extract (int n, char ** args)
{
    FILE * in = fopen(args[0], "rb");
    if (in == NULL) {
        errprintf("Could not open `%s`", args[0]);
        return;
    }

    struct star_file * star = star_read(in);
    fclose(in);
    if (star == NULL) {
        eprintf("An error occurred reading `%s`", args[0]);
        return;
    }

    if (n == 1) { /* extract every file */
        for (u64 fi = 0; fi < star->header.nfiles; fi++)
            _extract_file_id(star, fi);
    } else { /* extract only specified files */
        for (int i = 1; i < n; i++) {
            u64 id = star_search(star, (void *) args[i]);
            if (id != UINT64_MAX)
                _extract_file_id(star, id);
            else
                eprintf("No file named `%s` was found", args[i]);
        }
    }

    star_free(star);
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

int argv_strcmp (const void * _l, const void * _r)
{
    const char * l = * (const char **) _l;
    const char * r = * (const char **) _r;
    return star_strcmp(l, r);
}

int main (int argc, char ** argv)
{
    ifjmp(argc < 2, usage);

#define cmd(f, s, n) ((strcmp(argv[1], (s)) == 0) && argc >= (n)) ? (f)
    void (*todo) (int, char **) =
        cmd(create,  "c", 4) : /* star c archive file+ */
        cmd(extract, "x", 3) : /* star x archive file* */
        cmd(list,    "l", 3) : /* star l archive+      */
        NULL ;
#undef cmd

    ifjmp(todo == NULL, usage);

    qsort(argv + 3, argc - 3, sizeof(char *), argv_strcmp);

    todo(argc - 2, argv + 2);

    return EXIT_SUCCESS;

usage:
    usage(*argv);
    return EXIT_FAILURE;
}
