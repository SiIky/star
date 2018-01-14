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
    ifjmp(file == NULL, ko);
    ifjmp(self == NULL, ko);

    self->type = _STREAM_TYPE_FILE;
    self->s.f = file;

    return true;
ko:
    return false;
}

bool stream_from_raw (Stream * self, void * ptr, size_t size)
{
    ifjmp(self == NULL, ko);
    ifjmp(ptr == NULL, ko);
    ifjmp(size == 0, ko);

    self->type = _STREAM_TYPE_RAW;
    self->s.r.offset = 0;
    self->s.r.ptr = ptr;
    self->s.r.size = size;

    return true;
ko:
    return false;
}

size_t stream_read (Stream * self, void * out, size_t size, size_t nmemb)
{
    if (!_stream_check_type(self) || out == NULL)
        return 0;

    if (self->type == _STREAM_TYPE_FILE)
        return fread(out, size, nmemb, self->s.f);

    /* TODO: read elements, not bytes */
    /* _STREAM_TYPE_RAW */
    size_t want = size * nmemb;
    size_t have = (self->s.r.size > self->s.r.offset) ?
        (self->s.r.size - self->s.r.offset) :
        0 ;
    size_t bytes = min(want, have);

    memcpy(out, self->s.r.ptr, bytes);

    self->s.r.offset += bytes;

    return bytes;
}

size_t stream_write (Stream * self, const void * in, size_t size, size_t nmemb)
{
    if (!_stream_check_type(self) || in == NULL)
        return 0;

    if (self->type == _STREAM_TYPE_FILE)
        return fwrite(in, size, nmemb, self->s.f);

    /* TODO: read elements, not bytes */
    /* _STREAM_TYPE_RAW */
    size_t want = size * nmemb;
    size_t have = (self->s.r.size > self->s.r.offset) ?
        (self->s.r.size - self->s.r.offset) :
        0 ;
    size_t bytes = min(want, have);

    memcpy(self->s.r.ptr, in, bytes);

    self->s.r.offset += bytes;

    return bytes;
}

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
