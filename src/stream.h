#ifndef _STREAM_H
#define _STREAM_H

/*
 * <stdbool.h>
 *  bool
 *
 * <stdio.h>
 *  FILE
 *
 * <stdlib.h>
 *  size_t
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief The Stream type
 */
typedef struct {
    /** The type of Stream */
    enum {
        /** Invalid stream */
        _STREAM_TYPE_INVALID,
        /** Byte stream */
        _STREAM_TYPE_RAW,
        /** FILE stream */
        _STREAM_TYPE_FILE,
    } type;

    /** Where the data is held */
    union {
        /** The FILE, for a Stream of type _STREAM_TYPE_FILE */
        FILE * f;

        /** The data, for a Stream of type _STREAM_TYPE_RAW */
        struct {
            /** Size of the data, in bytes */
            size_t size;
            /** Where it will be reading/writing next */
            size_t offset;
            /** Pointer to the data */
            void * ptr;
        } r;
    } s;
} Stream;

/**
 * @brief Get a pointer to the FILE associated with @a self
 * @param self The Stream
 * @returns NULL if there's no FILE associated with @a self or @a self
 *     is not a FILE Stream, the pointer to the FILE otherwise
 */
FILE * stream_file (Stream * self);

/**
 * @brief Create a new Stream from @a file
 * @param self The Stream
 * @param file The FILE to associate with @a self
 * @returns `false` if either @a self or @a file are NULL, `true` otherwise
 */
bool stream_from_file (Stream * self, FILE * file);

/**
 * @brief Create a new Stream from @a ptr with @a size
 * @param self The Stream
 * @param ptr The data to associate with @a self
 * @param size The size of @a ptr, in bytes
 * @returns `false` if either @a self or @a ptr are NULL, or @a size
 *     is 0, `true` otherwise
 */
bool stream_from_raw (Stream * self, void * ptr, size_t size);

/**
 * @brief Similar to `fread()` from <stdio.h>, read @a nmemb elements
 *     of @a size to @a out, from @a self
 * @param self The Stream
 * @param out Where to write the data read
 * @param size Size of each element
 * @param nmemb Number of elements to read
 * @returns Number of elements read
 */
size_t stream_read (Stream * self, void * out, size_t size, size_t nmemb);

/**
 * @brief Similar to `fwrite()` from <stdio.h>, write @a nmemb elements
 *     of @a size to @a self, from @a in
 * @param self The Stream
 * @param in Where to read the data write
 * @param size Size of each element
 * @param nmemb Number of elements to write
 * @returns Number of elements written
 */
size_t stream_write (Stream * self, const void * in, size_t size, size_t nmemb);

/**
 * @brief Get a pointer to the data associated with @a self
 * @param self The Stream
 * @returns NULL if theres no data associated with @a self or @a self
 *     is a FILE Stream, the pointer to the data otherwise
 */
void * stream_raw (Stream * self);

/**
 * @brief Close @a self and, if @a self isn't a FILE Stream, `free()`
 *     associated data
 * @param self The Stream
 */
void stream_close (Stream * self);

#endif /* _STREAM_H */
