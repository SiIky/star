/*
 * <errno.h>
 *  errno
 *  strerror()
 *
 * <inttypes.h>
 *  PRIu64
 *
 * <stdio.h>
 *  FILE
 *  SEEK_END
 *  SEEK_SET
 *  fopen()
 *  fprintf()
 *  fseek()
 *  ftell()
 *  printf()
 *  stderr
 *
 * <stdlib.h>
 *  EXIT_FAILURE
 *  EXIT_SUCCESS
 *  qsort()
 *  size_t
 *
 * <string.h>
 *  strcmp()
 */
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * <utils/ifjmp.h>
 *  ifjmp()
 */
#include <utils/ifjmp.h>

#include "star.h"

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
            "\tList files in ARCHIVE.",
            cmd, cmd, cmd);
}

/* TODO: better file names handling */
int create (int _n, char ** args)
{
    int ret = EXIT_FAILURE;
    Stream out = {0};
    Stream in = {0};

    u64 n = (u64) _n; /* `_n > 0` */
    struct star_file * star = star_new(n - 1);
    ifjmp(star == NULL, out);

    for (u64 i = 1; i < n; i++) {
        if (!stream_from_file(&in, fopen(args[i], "rb"))) {
            errprintf("Error opening `%s`", args[i]);
            goto out;
        }

        u64 size = fsize(stream_file(&in));
        eprintf("Archiving `%s`", args[i]);

        if (!star_add_file(star, i - 1, (void *) args[i], size, &in)) {
            errprintf("Error adding file `%s`", args[i]);
            goto out;
        }

        stream_close(&in);
    }

    star_file_offsets(star);

    if (!stream_from_file(&out, fopen(args[0], "wb"))) {
        errprintf("Error opening `%s`", args[0]);
        goto out;
    }

    if (!star_write(star, &out)) {
        eprintf("Error writing STAR file `%s`", *args);
        goto out;
    }

    ret = EXIT_SUCCESS;

out:
    stream_close(&out);
    stream_close(&in);
    star_free(star);
    return ret;
}

#define _extract_file_id(S, ID) do {                      \
    Stream out = {0};                                     \
    if (!stream_from_file(&out,                           \
                fopen((void *) (S)->fheaders[(ID)].path,  \
                    "w+b")))                              \
    {                                                     \
        errprintf("Could not open `%s`",                  \
                (S)->fheaders[(ID)].path);                \
        break;                                            \
    }                                                     \
    eprintf("Extracting `%s`", (S)->fheaders[(ID)].path); \
    size_t w = stream_write(&out, (S)->fdata[(ID)],       \
            (S)->fheaders[(ID)].size, 1);                 \
    if (w != 1)                                           \
    eprintf("An error occurred writing `%s`",             \
            (S)->fheaders[(ID)].path);                    \
    stream_close(&out);                                   \
} while (0)

/* TODO: (maybe?) create file hierarchy (directories) */
int extract (int n, char ** args)
{
    Stream in = {0};
    if (!stream_from_file(&in, fopen(args[0], "rb"))) {
        eprintf("Error occurred opening `%s`", args[0]);
        return EXIT_FAILURE;
    }

    struct star_file * star = star_read(&in);
    stream_close(&in);
    if (star == NULL) {
        eprintf("Error occurred reading `%s`", args[0]);
        return EXIT_FAILURE;
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

    return EXIT_SUCCESS;
}

/* TODO: read headers only, not fdata */
int list (int n, char ** args)
{
    int ret = EXIT_SUCCESS;

    for (int i = 0; i < n; i++) {
        Stream in = {0};

        if (!stream_from_file(&in, fopen(args[i], "rb"))) {
            errprintf("Could not open `%s`", args[i]);
            ret = EXIT_FAILURE;
            stream_close(&in);
            continue;
        }

        struct star_file * star = star_read(&in);
        stream_close(&in);

        if (star == NULL) {
            eprintf("An error occurred reading `%s`", args[i]);
            ret = EXIT_FAILURE;
            continue;
        }

        printf("%s:\n", args[i]);
        for (u64 idx = 0; idx < star->header.nfiles; idx++)
            printf("\t`%s` (%" PRIu64 " B)\n",
                    star->fheaders[idx].path,
                    star->fheaders[idx].size);

        star_free(star);
    }

    return ret;
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
    int (*todo) (int, char **) =
        cmd(create,  "c", 4) : /* star c archive file+ */
        cmd(extract, "x", 3) : /* star x archive file* */
        cmd(list,    "l", 3) : /* star l archive+      */
        NULL ;
#undef cmd

    ifjmp(todo == NULL, usage);

    qsort(argv + 3, argc - 3, sizeof(char *), argv_strcmp);

    return todo(argc - 2, argv + 2);

usage:
    usage(*argv);
    return EXIT_FAILURE;
}
