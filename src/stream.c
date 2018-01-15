/*
 * <stdbool.h>
 *  bool
 *
 * <stdio.h>
 *  FILE
 *  fclose()
 *  fread()
 *  fwrite()
 *
 * <stdlib.h>
 *  free()
 *  size_t
 *
 * <string.h>
 *  memcpy()
 *  memset()
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * <utils/common.h>
 *  min()
 *
 * <utils/ifjmp.h>
 *  ifjmp()
 */
#include <utils/common.h>
#include <utils/ifjmp.h>

#include "stream.h"

/**
 * @brief Check if @a self is a valid Stream
 * @parm self The Stream
 * @returns `true` if @a self is a valid Stream, `false` otherwise
 */
static inline bool _stream_check_type (const Stream * self)
{
    return self != NULL
        && (self->type == _STREAM_TYPE_RAW
        || self->type == _STREAM_TYPE_FILE);
}

void stream_close (Stream * self)
{
    if (!_stream_check_type(self))
        return;

    if (self->type == _STREAM_TYPE_RAW) {
        if (self->s.r.ptr != NULL)
            free(self->s.r.ptr);
        memset(&self->s.r, 0, sizeof(self->s.r));
    } else {
        fclose(self->s.f);
        self->s.f = NULL;
    }

    self->type = _STREAM_TYPE_INVALID;
}

bool stream_from_file (Stream * self, FILE * file)
{
    if (file == NULL || self == NULL)
        return false;

    self->type = _STREAM_TYPE_FILE;
    self->s.f = file;

    return true;
}

bool stream_from_raw (Stream * self, void * ptr, size_t size)
{
    if (self == NULL || ptr == NULL || size == 0)
        return false;

    self->type = _STREAM_TYPE_RAW;
    self->s.r.offset = 0;
    self->s.r.ptr = ptr;
    self->s.r.size = size;

    return true;
}

/**
 * @brief Write at most @a pnmemb of @a psize from @a src to @a dst
 * @param dst The `dst` parameter of `memcpy()`
 * @param src The `src` parameter of `memcpy()`
 * @param ssize Total size the stream has available, in bytes
 * @param soff Offset of the stream
 * @param psize Size of each element to copy
 * @param pnmemb Number of elements wanted
 */
#define _stream_rw_raw(dst, src, ssize, soff, psize, pnmemb) \
    do {                                                     \
        size_t have_nmemb = ((ssize) - (soff)) / (psize);    \
        size_t nmemb2cp   = min((pnmemb), have_nmemb);       \
        size_t bytes2cp   = nmemb2cp * (psize);              \
        memcpy((dst), (src), bytes2cp);                      \
        (soff) += bytes2cp;                                  \
        return nmemb2cp;                                     \
    } while (0)

size_t stream_read (Stream * self, void * out, size_t size, size_t nmemb)
{
    if (!_stream_check_type(self) || out == NULL)
        return 0;

    if (self->type == _STREAM_TYPE_FILE)
        return fread(out, size, nmemb, self->s.f);

    /* _STREAM_TYPE_RAW */
    _stream_rw_raw(out, self->s.r.ptr, self->s.r.size,
            self->s.r.offset, size, nmemb);
}

size_t stream_write (Stream * self, const void * in, size_t size, size_t nmemb)
{
    if (!_stream_check_type(self) || in == NULL)
        return 0;

    if (self->type == _STREAM_TYPE_FILE)
        return fwrite(in, size, nmemb, self->s.f);

    /* _STREAM_TYPE_RAW */
    _stream_rw_raw(self->s.r.ptr, in, self->s.r.size,
            self->s.r.offset, size, nmemb);
}
#undef _stream_rw_raw

FILE * stream_file (Stream * self)
{
    return (_stream_check_type(self) && self->type == _STREAM_TYPE_FILE) ?
        self->s.f :
        NULL ;
}

void * stream_raw (Stream * self)
{
    return (_stream_check_type(self) && self->type == _STREAM_TYPE_RAW) ?
        self->s.r.ptr :
        NULL ;
}
