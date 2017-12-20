/*
 * <stdio.h>
 *  FILE
 *  fread()
 *  fseek()
 *  ftell()
 *  fwrite()
 *
 * <stdlib.h>
 *  calloc()
 *  free()
 *  malloc()
 *
 * <string.h>
 *  memcmp()
 *  memset()
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "star.h"

#define ifjmp(COND, LBL) if (COND) goto LBL

const unsigned char MAGIC[4] = { 0x53, 0x54, 0x41, 0x52 };

/*
 * utility functions
 */
bool star_check_header (const struct star_file * self)
{
    return (self != NULL) ?
        memcmp(self->header.magic, MAGIC, 4) == 0 :
        false ;
}

void star_free (struct star_file * self)
{
    if (self == NULL)
        return;

    size_t n = self->header.nfiles;

    if (self->fheaders != NULL) {
        for (size_t i = 0; i < n; i++)
            if (self->fheaders[i].path != NULL)
                free(self->fheaders[i].path);

        free(self->fheaders);
    }

    if (self->fdata != NULL) {
        for (size_t i = 0; i < n; i++)
            if (self->fdata[i] != NULL)
                free(self->fdata[i]);

        free(self->fdata);
    }

    free(self);
    memset(self, 0, sizeof(struct star_file));
}

/*
 * read functions (assume `in` was opened in read mode)
 */

bool star_read_header (struct star_file * self, FILE * in)
{
    bool ret = false;

    ifjmp(self == NULL, out);
    ifjmp(in == NULL, out);

    size_t r = fread(&self->header, sizeof(struct star_header), 1, in);
    ifjmp(r != 1, out);

    ret = star_check_header(self);

out:
    return ret;
}

size_t star_read_fheaders (struct star_file * self, FILE * in)
{
    size_t ret = 0;

    ifjmp(self == NULL, out);
    ifjmp(in == NULL, out);

    size_t nfiles = self->header.nfiles;

    struct star_file_header * fheaders = (self->fheaders == NULL) ?
        calloc(self->header.nfiles, sizeof(struct star_file_header)) :
        self->fheaders ;
    ifjmp(fheaders == NULL, out);

    /* assume `fheaders` has enough space  */
    for (ret = 0; ret < nfiles; ret++) {
        /*
         * dont change original memory if possible,
         * write only at the end of the loop
         */
        struct star_file_header tmp;

        { /* read `size,` `offset` and `path_len` */
            void * ptr = &tmp;
            size_t size = sizeof(size_t);
            size_t nmemb = 3;

            size_t r = fread(ptr, size, nmemb, in);

            if (r != nmemb)
                break;
        }

        { /* alloc path */
            tmp.path = malloc(tmp.path_len);

            if (tmp.path == NULL)
                break;
        }

        { /* read path */
            void * ptr = tmp.path;
            size_t size = sizeof(unsigned char);
            size_t nmemb = tmp.path_len;
            size_t r = fread(ptr, size, nmemb, in);

            /* we alloc so we have to free in case of error */
            if (r != nmemb) {
                free(ptr);
                break;
            }
        }

        fheaders[ret] = tmp;
    }

    self->fheaders = fheaders;

out:
    return ret;
}

size_t star_read_fdata (struct star_file * self, FILE * in)
{
    size_t ret = 0;

    ifjmp(self == NULL, out);
    ifjmp(in == NULL, out);

    unsigned char ** fdata = (self->fdata == NULL) ?
        calloc(self->header.nfiles, sizeof(unsigned char *)) :
        self->fdata ;
    ifjmp(fdata == NULL, out);

    /* assume `fdata` has enough space */
    for (ret = 0; ret < self->header.nfiles; ret++) {
        unsigned char * tmp = NULL;

        size_t size = self->fheaders[ret].size;
        tmp = malloc(size);
        if (tmp == NULL)
            break;

        size_t r = fread(tmp, size, 1, in);

        /* we alloc so we have to free in case of error */
        if (r != 1) {
            free(fdata[ret]);
            break;
        }

        fdata[ret] = tmp;
    }

    self->fdata = fdata;

out:
    return ret;
}

struct star_file * star_read (FILE * in)
{
    struct star_file * ret = NULL;
    struct star_file _tmp;
    memset(&_tmp, 0, sizeof(struct star_file));

    /* failed to read header or `in` is not a STAR file */
    ifjmp(!star_read_header(&_tmp, in), out);

    {
        ret = malloc(sizeof(struct star_file));
        ifjmp(ret == NULL, out);

        *ret = _tmp;
    }

    /* number of successfully read file headers */
    size_t nfheaders = star_read_fheaders(ret, in);
    ifjmp(ret->header.nfiles != nfheaders, ko_fheaders);

    /* number of successfully read files */
    size_t nfdata = star_read_fdata(ret, in);
    ifjmp(ret->header.nfiles != nfdata, ko_fdata);

out:
    return ret;

ko_fdata:
    for (size_t i = 0; i < nfdata; i++)
        free(ret->fdata[i]);
    free(ret->fdata);

ko_fheaders:
    for (size_t i = 0; i < nfheaders; i++)
        free(ret->fheaders[i].path);
    free(ret->fheaders);

    free(ret);
    ret = NULL;
    goto out;
}

/*
 * write functions (assume `out` was opened in write mode)
 */

#define fwrite_ifjmp(PTR, SIZE, NMEMB, STREAM, LBL) \
    ifjmp((fwrite((PTR), (SIZE), (NMEMB), (STREAM)) != (NMEMB)), LBL)

bool star_write (const struct star_file * self, FILE * out)
{
    bool ret = false;

    ifjmp(self == NULL, out);
    ifjmp(out == NULL, out);
    ifjmp(!star_check_header(self), out);
    ifjmp(self->fheaders == NULL, out);
    ifjmp(self->fdata == NULL, out);

    /* assume both `self->fheaders` and `self->fdata` have the correct size */
    for (size_t i = 0; i < self->header.nfiles; i++) {
        ifjmp(self->fdata[i] == NULL, out); // BUG
        ifjmp(self->fheaders[i].path == NULL, out);
        ifjmp((strlen((void *) self->fheaders[i].path) + 1) != self->fheaders[i].path_len, out);
    }

    /* write STAR header */
    fwrite_ifjmp(&self->header,
                 sizeof(struct star_header),
                 1,
                 out,
                 out);

    /* write file headers */
    for (size_t i = 0; i < self->header.nfiles; i++) {
        /* write `size`, `offset` and `path_len` */
        fwrite_ifjmp(self->fheaders + i,
                     sizeof(size_t),
                     3,
                     out,
                     out);

        /* write `path` */
        fwrite_ifjmp(self->fheaders[i].path,
                     sizeof(unsigned char),
                     self->fheaders[i].path_len,
                     out,
                     out);
    }

    /* write file data */
    for (size_t i = 0; i < self->header.nfiles; i++)
        fwrite_ifjmp(self->fdata[i],
                     self->fheaders[i].size,
                     1,
                     out,
                     out);

    ret = true;

out:
    return ret;
}

#undef fwrite_ifjmp

/*
 * create functions
 */
struct star_file * star_new (size_t nfiles)
{
    struct star_file _tmp;
    struct star_file * ret = NULL;

    memset(&_tmp, 0, sizeof(struct star_file));

    ifjmp(nfiles == 0, out);

    _tmp.fdata = calloc(nfiles, sizeof(unsigned char *));
    _tmp.fheaders = calloc(nfiles, sizeof(struct star_file_header));
    ret = malloc(sizeof(struct star_file));

    ifjmp(_tmp.fdata == NULL, ko);
    ifjmp(_tmp.fheaders == NULL, ko);
    ifjmp(ret == NULL, ko);

    memcpy(&_tmp.header.magic, MAGIC, 4);
    _tmp.header.nfiles = nfiles;

    *ret = _tmp;

out:
    return ret;

ko:
    if (ret != NULL)
        free(ret);

    if (_tmp.fdata != NULL)
        free(_tmp.fdata);

    if (_tmp.fheaders != NULL)
        free(_tmp.fheaders);

    goto out;
}

bool star_add_file (struct star_file * self, size_t idx, const char * path, size_t size, FILE * in)
{
    bool ret = false;
    unsigned char * fdata = NULL;
    struct star_file_header fheader;
    memset(&fheader, 0, sizeof(struct star_file_header));

    ifjmp(self == NULL, out);
    ifjmp(in == NULL, out);
    ifjmp(path == NULL, out);
    ifjmp(idx >= self->header.nfiles, out);

    { /* file data */
        fdata = malloc(size);
        ifjmp(fdata == NULL, out);

        size_t r = fread(fdata, size, 1, in);
        ifjmp(r != 1, ko);
    }

    { /* path */
        fheader.path = (void *) strdup(path);
        ifjmp(fheader.path == NULL, ko);
        fheader.path_len = strlen((void *) fheader.path) + 1;
    }

    fheader.size = size;

    self->fdata[idx] = fdata;
    self->fheaders[idx] = fheader;

    ret = true;

out:
    return ret;

ko:
    if (fdata != NULL)
        free(fdata);

    if (fheader.path != NULL)
        free(fheader.path);

    goto out;
}

/*
 * assume self is a complete `struct star_file`, ready to
 * write, except for the file offsets
 */
bool star_file_offsets (struct star_file * self)
{
    bool ret = false;

    ifjmp(self == NULL, out);

    size_t offset = 0;

    /*
     * offset from the beggining of the STAR file to the
     * beggining of stored files' data
     */
    {
        offset += sizeof(struct star_header);
        offset += self->header.nfiles * sizeof(struct star_file_header);

        for (size_t i = 0; i < self->header.nfiles; i++)
            offset += self->fheaders[i].path_len;

        offset -= self->header.nfiles * sizeof(unsigned char *);
    }

    self->fheaders[0].offset = offset;

    for (size_t i = 1; i < self->header.nfiles; i++)
        self->fheaders[i].offset = self->fheaders[i - 1].offset;

    ret = true;

out:
    return ret;
}
