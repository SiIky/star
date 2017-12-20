#ifndef _STAR_H
#define _STAR_H

/*
 * <stdbool.h>
 *  bool
 *
 * <stddef.h>
 *  size_t
 */
#include <stdbool.h>
#include <stddef.h>

extern const unsigned char MAGIC[4];

struct star_header {
    unsigned char magic[4];
    size_t nfiles;
};

struct star_file_header {
    size_t size;
    size_t offset;
    size_t path_len; /* including terminating '\0' */
    unsigned char * path;
};

struct star_file {
    struct star_header header;
    struct star_file_header * fheaders;
    unsigned char ** fdata;
};

/* utility functions */
bool star_check_header (const struct star_file * self);
void star_free         (struct star_file * self);

/* read functions */
bool               star_read_header      (struct star_file * self, FILE * in);
size_t             star_read_fdata       (struct star_file * self, FILE * in);
size_t             star_read_fheaders    (struct star_file * self, FILE * in);
struct star_file * star_read             (FILE * in);

/* write functions */
bool star_write (const struct star_file * self, FILE * out);

/* create functions */
bool               star_add_file     (struct star_file * self, size_t idx, const char * path, size_t size, FILE * in);
bool               star_file_offsets (struct star_file * self);
struct star_file * star_new          (size_t nfiles);

#endif /* _STAR_H */
