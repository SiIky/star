#ifndef _STAR_H
#define _STAR_H

/*
 * <stdbool.h>
 *  bool
 *
 * <stddef.h>
 *  uint_least64_t
 *  uint_least8_t
 */
#include <stdbool.h>
#include <stdint.h>

/*
 * exact size ints names are too dam long;
 * not required by C11, but supported at least by
 * GLibC and Musl
 */
typedef uint64_t u64;
typedef uint8_t  u8;
typedef int8_t   i8;

extern const u8 MAGIC[4];

struct star_header {
    u8 magic[4];
    u64 nfiles;
};

struct star_file_header {
    u64 size;
    u64 offset;
    u64 path_len; /* including terminating '\0' */
    u8 * path;
};

struct star_file {
    struct star_header header;
    struct star_file_header * fheaders;
    u8 ** fdata;
};

/* utility functions */
bool star_check_header (const struct star_file * self);
int  star_strcmp       (const void * l, const void * r);
void star_free         (struct star_file * self);

/* read functions */
bool               star_read_header   (struct star_file * self, FILE * in);
u64                star_read_fdata    (struct star_file * self, FILE * in);
u64                star_read_fheaders (struct star_file * self, FILE * in);
struct star_file * star_read          (FILE * in);

/* write functions */
bool star_write (const struct star_file * self, FILE * out);

/* create functions */
bool               star_add_file     (struct star_file * self, u64 idx, const u8 * path, u64 size, FILE * in);
bool               star_file_offsets (struct star_file * self);
struct star_file * star_new          (u64 nfiles);

#endif /* _STAR_H */
