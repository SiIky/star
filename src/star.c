/*
 * <limits.h>
 *  CHAR_BIT
 *  UCHAR_MAX
 *
 * <stdio.h>
 *  FILE
 *  fread()
 *  fseek()
 *  ftell()
 *  fwrite()
 *
 * <stdlib.h>
 *  bsearch()
 *  calloc()
 *  free()
 *  malloc()
 *
 * <string.h>
 *  memcmp()
 *  strcmp()
 *  strlen()
 *  strncmp()
 */
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * <utils/ifjmp.h>
 *  ifjmp()
 */
#include <utils/ifjmp.h>

#include "star.h"

#define star_max(x, y) (((x) > (y)) ? (x) : (y))
#define star_min(x, y) (((x) < (y)) ? (x) : (y))

static const u8 STAR_MAGIC[4] = { 0x53, 0x54, 0x41, 0x52 };

/***********************************************************
 * utility functions
 **********************************************************/

static inline bool _star_check_ptrs (const struct star_file * self)
{
    bool ret = false;

    ifjmp(self == NULL, out);
    ifjmp(self->fheaders == NULL, out);
    ifjmp(self->fdata == NULL, out);

    ret = true;

    for (u64 i = 0; i < self->header.nfiles && ret; i++)
        ret = self->fheaders[i].path != NULL
            && self->fdata[i] != NULL;

out:
    return ret;
}

/*
 * `_star_uint_width_encode()` and `_star_uint_width_decode()`
 * from http://www.iso-9899.info/wiki/Temp
 */

/**
 * @brief Serialize @a in with @a width into @a out
 * @param out Where to serialize to
 * @param in The integer to serialize
 * @param width The witdh of the integer to serialize
 */
static void _star_uint_width_encode (u8 * out, u64 in, size_t width)
{
    if (width > sizeof(u64)) return;
    for (size_t i = 0; i < width; i++)
        out[i] = (in >> (i * CHAR_BIT)) & UCHAR_MAX;
}

/**
 * @brief Deserialize @a out from @a in with @a width
 * @param out Where to deserialize to
 * @param in The data to deserialize from
 * @param width The witdh of the integer to deserialize
 */
static void _star_uint_width_decode (u64 * out, u8 * in, size_t width)
{
    if (width > sizeof(u64)) return;
    *out = 0;
    for (size_t i = 0; i < width; i++)
        *out |= (u64) in[i] << (i * CHAR_BIT);
}

bool star_check_header (const struct star_file * self)
{
    return (self != NULL)
        && (memcmp(self->header.magic, STAR_MAGIC,
                    sizeof(STAR_MAGIC)) == 0);
}

//
// This function works only on certain circumstances.
//
// # Use case:
//
// Given a directory tree similar to the following:
//
//   ```
//   directory/
//   ├── file1
//   ├── file2
//   ├── file3
//   ├── file4
//   ├── file5
//   ├── file6
//   ├── file7
//   ├── file8
//   ├── file9
//   ├── file10
//   └── file11
//   ```
//
// and calling a `program` this way:
//
//   ```sh
//   program directory/*
//   ```
//
// `argv` will look like:
//
//   ```c
//   argv = {
//       [0]  = "program",
//       [1]  = "directory/file1",
//       [2]  = "directory/file10",
//       [3]  = "directory/file11",
//       [4]  = "directory/file2",
//       [5]  = "directory/file3",
//       [6]  = "directory/file4",
//       [7]  = "directory/file5",
//       [8]  = "directory/file6",
//       [9]  = "directory/file7",
//       [10] = "directory/file8",
//       [11] = "directory/file9",
//   }
//   ```
//
// This isn't ideal if you want to process these files in
// order.
//
// # Assumptions
//
// The strings to be compared are composed of common a
// prefix and a number after that prefix (`pn`). It
// doesn't matter what the prefix is.
//
// e.g.:
//   pre1 < pre2 < pre10
//
// # Limitations
//
// The numbers aren't read nor compared as numbers, so the
// same number with a different representation will not
// give the correct order.
//
// e.g.:
//   1 < 01 < 001
//   2 < 01 < 000
//
int star_strcmp (const void * _l, const void * _r)
{
    const char * l = _l;
    const char * r = _r;

    /* `l.len == r.len` <=> `l.len - r.len == 0` */
    size_t nl = strlen(l);
    size_t nr = strlen(r);
    int diff = nl - nr;

    return (diff == 0) ?
        strcmp(l, r) :
        diff ;
}

void star_free (struct star_file * self)
{
    if (self == NULL)
        return;

    u64 n = self->header.nfiles;

    if (self->fheaders != NULL) {
        for (u64 i = 0; i < n; i++)
            if (self->fheaders[i].path != NULL)
                free(self->fheaders[i].path);

        free(self->fheaders);
    }

    if (self->fdata != NULL) {
        for (u64 i = 0; i < n; i++)
            if (self->fdata[i] != NULL)
                free(self->fdata[i]);

        free(self->fdata);
    }

    free(self);
}

/***********************************************************
 * read functions (assume `in` was opened in read mode)
 **********************************************************/

/*
 * macro to define functions to read specific unsigned integer types
 */
#define _star_make_read_fun(name, type)                       \
    bool name (type * out, FILE * in, u64 nmemb) {            \
        ifjmp(in == NULL, ko);                                \
        ifjmp(out == NULL, ko);                               \
        u8 buf[sizeof(type)] = {0};                           \
        for (u64 i = 0; i < nmemb; i++) {                     \
            u64 r = fread(buf, sizeof(type), 1, in);          \
            ifjmp(r != 1, ko);                                \
            u64 tmp = 0;                                      \
            _star_uint_width_decode(&tmp, buf, sizeof(type)); \
            *out = (type) tmp; /* safe because of above */    \
        }                                                     \
        return true;                                          \
        ko:                                                   \
        return false;                                         \
    }

/*
 * define functions to read specific unsigned integer types
 */
_star_make_read_fun(star_read_u64, u64)

/* read SIZE bytes from OUT int IN, as if they were a single unit */
#define star_read_u8_single(OUT, IN, SIZE) \
        (fread((OUT), (SIZE), 1, (IN)) == 1)

bool star_read_header (struct star_file * self, FILE * in)
{
    bool ret = false;

    ifjmp(self == NULL, out);
    ifjmp(in == NULL, out);

    struct star_file tmp = {0};

    ifjmp(!star_read_u8_single(&tmp.header.magic, in, sizeof(STAR_MAGIC)), out);
    ifjmp(!star_check_header(&tmp), out);
    ifjmp(!star_read_u64(&tmp.header.nfiles, in, 1), out);

    self->header = tmp.header;
    ret = true;
out:
    return ret;
}

bool star_read_fheader (struct star_file_header * fheader, FILE * in)
{
    ifjmp(fheader == NULL, ko);
    ifjmp(in == NULL, ko);

    star_read_u64(&fheader->size, in, 1);
    star_read_u64(&fheader->offset, in, 1);
    star_read_u64(&fheader->path_len, in, 1);

    fheader->path = malloc(fheader->path_len);
    ifjmp(fheader->path == NULL, ko);

    if (!star_read_u8_single(fheader->path, in, fheader->path_len)) {
        free(fheader->path);
        fheader->path = NULL;
        goto ko;
    }

    return true;
ko:
    return false;
}

u64 star_read_fheaders (struct star_file * self, FILE * in)
{
    u64 ret = 0;

    ifjmp(self == NULL, out);
    ifjmp(in == NULL, out);

    u64 nfiles = self->header.nfiles;

    /* if fheaders already has memory, use it */
    struct star_file_header * fheaders = (self->fheaders == NULL) ?
        calloc(self->header.nfiles, sizeof(struct star_file_header)) :
        self->fheaders ;
    ifjmp(fheaders == NULL, out);

    /* assume `fheaders` has enough space  */
    for (ret = 0; ret < nfiles; ret++) {
        struct star_file_header tmp = {0};

        /* wasnt able to read the fheader? */
        if (!star_read_fheader(&tmp, in))
            break;

        fheaders[ret] = tmp;
    }

    self->fheaders = fheaders;

out:
    return ret;
}

u64 star_read_fdata (struct star_file * self, FILE * in)
{
    u64 ret = 0;

    ifjmp(self == NULL, out);
    ifjmp(in == NULL, out);

    u8 ** fdata = (self->fdata == NULL) ?
        calloc(self->header.nfiles, sizeof(u8 *)) :
        self->fdata ;
    ifjmp(fdata == NULL, out);

    /* assume `fdata` has enough space */
    for (ret = 0; ret < self->header.nfiles; ret++) {
        fdata[ret] = malloc(self->fheaders[ret].size);
        if (fdata[ret] == NULL)
            break;

        if (!star_read_u8_single(fdata[ret], in, self->fheaders[ret].size)) {
            free(fdata[ret]);
            fdata[ret] = NULL;
            break;
        }
    }

    self->fdata = fdata;

out:
    return ret;
}

struct star_file * star_read (FILE * in)
{
    struct star_file * ret = NULL;
    struct star_file tmp = {0};

    /* failed to read header or `in` is not a STAR file */
    ifjmp(!star_read_header(&tmp, in), out);

    { /* allocate and copy the already read data */
        ret = malloc(sizeof(struct star_file));
        ifjmp(ret == NULL, out);
        *ret = tmp;
    }

    /*
     * if it wasnt possible to read everything, return NULL
     */
    /* number of successfully read file headers */
    u64 nfheaders = star_read_fheaders(ret, in);
    ifjmp(ret->header.nfiles != nfheaders, ko_fheaders);

    /* number of successfully read files */
    u64 nfdata = star_read_fdata(ret, in);
    ifjmp(ret->header.nfiles != nfdata, ko_fdata);

out:
    return ret;

ko_fdata:
    for (u64 i = 0; i < nfdata; i++)
        free(ret->fdata[i]);
    free(ret->fdata);

ko_fheaders:
    for (u64 i = 0; i < nfheaders; i++)
        free(ret->fheaders[i].path);
    free(ret->fheaders);

    free(ret);
    ret = NULL;
    goto out;
}

/***********************************************************
 * write functions (assume `out` was opened in write mode)
 **********************************************************/

/*
 * macro to define functions to write specific unsigned integer types
 */
#define _star_make_write_fun(name, type)                       \
    bool name (const type * in, FILE * out, u64 nmemb) {       \
        ifjmp(in == NULL, ko);                                 \
        ifjmp(out == NULL, ko);                                \
        u8 buf[sizeof(type)] = {0};                            \
        for (u64 i = 0; i < nmemb; i++) {                      \
            _star_uint_width_encode(buf, in[i], sizeof(type)); \
            u64 w = fwrite(buf, sizeof(type), 1, out);         \
            ifjmp(w != 1, ko);                                 \
        }                                                      \
        return true;                                           \
    ko:                                                        \
        return false;                                          \
    }

/*
 * define functions to write specific unsigned integer types
 */
_star_make_write_fun(star_write_u64, u64)

#define star_write_u8_single(IN, OUT, SIZE) \
        (fwrite((IN), (SIZE), 1, (OUT)) == 1)

bool star_write_header (const struct star_file * self, FILE * out)
{
    ifjmp(self == NULL, ko);
    ifjmp(out == NULL, ko);

    ifjmp(!star_write_u8_single(STAR_MAGIC, out, sizeof(STAR_MAGIC)), ko);
    ifjmp(!star_write_u64(&self->header.nfiles, out, 1), ko);

    return true;
ko:
    return false;
}

#define try(cond, stmt) if (cond) { (cond) = (stmt); }
#define try_jmp(cond, lbl, stmt) \
    try((cond), (stmt)) else { goto lbl; }

bool star_write_fheader (const struct star_file_header * fh, FILE * out)
{
    bool ret = false;

    ifjmp(fh == NULL, ko);
    ifjmp(out == NULL, ko);

    ret = true;

    try_jmp(ret, ko, star_write_u64(&fh->size,     out, 1));
    try_jmp(ret, ko, star_write_u64(&fh->offset,   out, 1));
    try_jmp(ret, ko, star_write_u64(&fh->path_len, out, 1));
    try_jmp(ret, ko, star_write_u8_single(fh->path, out, fh->path_len));

    return ret;
ko:
    return false;
}

bool star_write_fheaders (const struct star_file * self, FILE * out)
{
    bool ret = false;

    ifjmp(self == NULL, ko);
    ifjmp(out == NULL, ko);

    ret = true;
    for (u64 i = 0; i < self->header.nfiles && ret; i++)
        ret = star_write_fheader(self->fheaders + i, out);

ko:
    return ret;
}

bool star_write_fdata (const struct star_file * self, FILE * out)
{
    ifjmp(self == NULL, ko);
    ifjmp(out == NULL, ko);
    ifjmp(self->fheaders == NULL, ko);
    ifjmp(self->fdata == NULL, ko);

    u64 i = 0;
    for (i = 0; i < self->header.nfiles; i++)
        if (!star_write_u8_single(self->fdata[i], out, self->fheaders[i].size))
            break;

    return i == self->header.nfiles;
ko:
    return false;
}

bool star_write (const struct star_file * self, FILE * out)
{
    ifjmp(!_star_check_ptrs(self), ko);
    ifjmp(out == NULL, ko);
    ifjmp(!star_check_header(self), ko);

    ifjmp(!star_write_header(self, out), ko);
    ifjmp(!star_write_fheaders(self, out), ko);
    ifjmp(!star_write_fdata(self, out), ko);

    return true;
ko:
    return false;
}

/***********************************************************
 * create functions
 **********************************************************/
struct star_file * star_new (u64 nfiles)
{
    struct star_file * ret = NULL;
    struct star_file _tmp = {0};

    ifjmp(nfiles == 0, out);

    _tmp.fdata = calloc(nfiles, sizeof(u8 *));
    _tmp.fheaders = calloc(nfiles, sizeof(struct star_file_header));
    ret = malloc(sizeof(struct star_file));

    ifjmp(_tmp.fdata == NULL, ko);
    ifjmp(_tmp.fheaders == NULL, ko);
    ifjmp(ret == NULL, ko);

    memcpy(&_tmp.header.magic, STAR_MAGIC, sizeof(STAR_MAGIC));
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

bool star_add_file (struct star_file * self, u64 idx, const u8 * path, u64 size, FILE * in)
{
    bool ret = false;
    u8 * fdata = NULL;
    struct star_file_header fheader = {0};

    ifjmp(self == NULL, out);
    ifjmp(in == NULL, out);
    ifjmp(path == NULL, out);
    ifjmp(idx >= self->header.nfiles, out);

    { /* file data */
        fdata = malloc(size);
        ifjmp(fdata == NULL, out);

        u64 r = fread(fdata, size, 1, in);
        ifjmp(r != 1, ko);
    }

    { /* path */
        fheader.path = (void *) strdup((void *) path);
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
    if (self == NULL)
        return false;

    u64 offset = 0;

    /*
     * offset from the beggining of the STAR file to the
     * beggining of stored files' data
     */
    {
        offset += sizeof(struct star_header);
        offset += self->header.nfiles * sizeof(struct star_file_header);

        /* add path length */
        for (u64 i = 0; i < self->header.nfiles; i++)
            offset += self->fheaders[i].path_len;

        /* subtract size of pointer to path */
        offset -= self->header.nfiles * sizeof(self->fheaders->path);
    }

    /* beggining of fdata */
    self->fheaders[0].offset = offset;
    for (u64 i = 0; i < self->header.nfiles - 1; i++)
        self->fheaders[i + 1].offset = self->fheaders[i].offset + self->fheaders[i].size;

    return true;
}

/***********************************************************
 * search functions
 **********************************************************/
u64 star_search (const struct star_file * self, const u8 * fname)
{
    bool match = false;
    u64 ret = 0;

    ifjmp(self == NULL, out);
    ifjmp(self->fheaders == NULL, out);
    ifjmp(fname == NULL, out);

    size_t fl = strlen((void *) fname);

    for (ret = 0; ret < self->header.nfiles && !match; ret++) {
        u64 n = self->fheaders[ret].path_len - 1;
        match = (n == fl) ?
            strncmp((void *) fname,
                    (void *) self->fheaders[ret].path, n) == 0 :
            match ;
    }

out:
    return (!match) ?
        STAR_DNF :
        ret ;
}

/**
 * @brief Compare @a path with a file header @a fh
 * @param path Path to look for on @a fh
 * @param fh A file header
 * @returns Similar to `strcmp()`
 */
static int _star_compar_path (const void * path, const void * fh)
{
    return (path != NULL && fh != NULL) ?
        star_strcmp(path, ((const struct star_file_header *) fh)->path) :
        0 ;
}

/* FIXME: Wrong matches, sometimes SEGV */
u64 star_bsearch (const struct star_file * self, const u8 * fname)
{
    u64 ret = STAR_DNF;

    ifjmp(self == NULL, out);
    ifjmp(self->fheaders == NULL, out);
    ifjmp(fname == NULL, out);

    const void * key = fname;
    const struct star_file_header * base = self->fheaders;
    size_t nmemb = self->header.nfiles;
    size_t size = sizeof(struct star_file_header);
    const struct star_file_header * match = bsearch(key, base, nmemb, size, _star_compar_path);

    ret = (match != NULL) ?
        (u64) (match - base) :
        STAR_DNF;

out:
    return ret;
}
