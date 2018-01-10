#ifndef _STAR_H
#define _STAR_H

/*
 * <stdbool.h>
 *  bool
 *
 * <stdint.h>
 *  UINT64_MAX
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

/**
 * @brief A STAR header
 */
struct star_header {
    /** Magic to check if data read is a STAR archive */
    u8 magic[4];
    /** Number of files the STAR archive contains */
    u64 nfiles;
};

/**
 * @brief Header of a file archived in a STAR archive
 */
struct star_file_header {
    /** Size of the file, in bytes */
    u64 size;
    /** Offset from the beggining of the STAR to the beggining of the file */
    u64 offset;
    /** Number of bytes of the path/filename, including terminating `NULL` byte */
    u64 path_len; /* including terminating '\0' */
    /** Path/filename of the file (no encoding assumed, just a sequence of bytes) */
    u8 * path;
};

/**
 * @brief A STAR
 */
struct star_file {
    /** The STAR header */
    struct star_header header;
    /** Headers of the archived files */
    struct star_file_header * fheaders;
    /** File data */
    u8 ** fdata;
};

/***********************************************************
 * utility functions
 **********************************************************/

/**
 * @brief Check the magic of @a self
 * @param self A STAR
 * @returns `true` if magic checks out, `false` otherwise
 */
bool star_check_header (const struct star_file * self);

/**
 * @brief Fake natural sorting
 * @param l String to compare
 * @param r String to compare
 * @returns Similar to `strcmp()`
 */
int  star_strcmp       (const void * l, const void * r);

/**
 * @brief Free @a self
 * @param self A STAR to free
 */
void star_free         (struct star_file * self);

/***********************************************************
 * read functions
 **********************************************************/

/**
 * @brief Read the STAR header from @a in to @a self
 * @param self STAR to read to
 * @param in A FILE stream opened with the "rb" mode and positioned at the beggining of a STAR header
 * @returns `true` if it successfully read the header and the magic checks out, `false` otherwise
 */
bool               star_read_header   (struct star_file * self, FILE * in);

/**
 * @brief Read archived file data from @a in to @a self
 * @param self STAR to read to
 * @param in A FILE stream opened with the "rb" mode and positioned at the beggining of the first archived file's data
 * @returns Number of files read, or `0` if an error occurred
 */
u64                star_read_fdata    (struct star_file * self, FILE * in);

/**
 * @brief Read archived file headers from @a in to @a self
 * @param self STAR to read to
 * @param in A FILE stream opened with the "rb" mode and positioned at the beggining of the first archived file header
 * @returns Number of file headers read, or `0` if an error occurred
 */
u64                star_read_fheaders (struct star_file * self, FILE * in);

/**
 * @brief Read a STAR from @a in
 * @param in A FILE stream opened with the "rb" mode and positioned at the beggining of a STAR header
 * @returns A pointer to a STAR, or `NULL` if an error occurred
 */
struct star_file * star_read          (FILE * in);

/***********************************************************
 * write functions
 **********************************************************/

/**
 * @brief Write @a self to @a out
 * @param self A STAR
 * @param out A FILE stream opened with the "wb" mode
 * @returns `true` if @a self was successfully written to @a out, `false` otherwise
 */
bool star_write (const struct star_file * self, FILE * out);

/***********************************************************
 * create functions
 **********************************************************/

/**
 * @brief Add a file to a STAR
 * @param self A STAR
 * @param idx Index where to add given file
 * @param path The filename of the file to add
 * @param size Size of the file, in bytes
 * @param in A FILE stream opened with the "rb" mode and positioned at the start of the data to be read
 * @returns `true` if the file was successfully added, `false` otherwise
 */
bool               star_add_file     (struct star_file * self, u64 idx, const u8 * path, u64 size, FILE * in);

/**
 * @brief Calculate the offsets of all the archived files in @a self
 * @param self A STAR, ready to be written, except for the file offsets
 * @returns `false` if an error occurred, `true` otherwise
 */
bool               star_file_offsets (struct star_file * self);

/**
 * @brief Create a new STAR with @a nfiles
 * @param nfiles Number of files the STAR is expected to hold
 * @returns A pointer to a STAR, or `NULL` if an error occurred
 */
struct star_file * star_new          (u64 nfiles);

/***********************************************************
 * search functions
 **********************************************************/

/**
 * @brief Search for an archived file named @a fname in @a self
 * @param self A STAR
 * @param fname The filename to search
 * @param The index of the searched file, `STAR_DNF` otherwise
 */
u64 star_search  (const struct star_file * self, const u8 * fname);

/**
 * @brief Binary Search for an archived file named @a fname in @a self
 * @param self A STAR with files sorted by filename
 * @param fname The filename to search
 * @param The index of the searched file, `STAR_DNF` otherwise
 */
u64 star_bsearch (const struct star_file * self, const u8 * fname);

#endif /* _STAR_H */
