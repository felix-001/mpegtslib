// Last Update:2019-04-02 09:44:01
/**
 * @file bitstream.h
 * @brief 
 * @author liyq
 * @version 0.1.00
 * @date 2018-10-27
 */

#ifndef BITSTREAM_H
#define BITSTREAM_H

#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>

#define av_assert0 assert
#define av_assert2 assert
#define av_log( a, b, args... ) printf(args)
#define av_unused

#   define AV_WB32(p, darg) do {                \
         unsigned d = (darg);                    \
         ((uint8_t*)(p))[3] = (d);               \
         ((uint8_t*)(p))[2] = (d)>>8;            \
         ((uint8_t*)(p))[1] = (d)>>16;           \
         ((uint8_t*)(p))[0] = (d)>>24;           \
     } while(0)

#   define AV_WL32(p, darg) do {                \
         unsigned d = (darg);                    \
         ((uint8_t*)(p))[0] = (d);               \
         ((uint8_t*)(p))[1] = (d)>>8;            \
         ((uint8_t*)(p))[2] = (d)>>16;           \
         ((uint8_t*)(p))[3] = (d)>>24;           \
     } while(0)

typedef struct bit_stream {
    int size_in_bits;
    uint32_t bit_buf;
    int bit_left;
    uint8_t *buf, *buf_ptr, *buf_end;
} bit_stream_t;

static inline void bs_init( bit_stream_t *bs, uint8_t *buffer, int buffer_size )
{
    if (!bs ) {
        return;
    }

    bs->size_in_bits = 8 * buffer_size;
    bs->buf          = buffer;
    bs->buf_end      = bs->buf + buffer_size;
    bs->buf_ptr      = bs->buf;
    bs->bit_left     = 32;
    bs->bit_buf      = 0;

}

static inline void bs_write( bit_stream_t *bs, int value, int n )
{
    unsigned int bit_buf;
    int bit_left;

    av_assert2(n <= 31 );
    av_assert2(value < (1U << n) );
    av_assert2(n <= 31 && value < (1U << n));

    bit_buf  = bs->bit_buf;
    bit_left = bs->bit_left;

#ifdef BITSTREAM_WRITER_LE
    bit_buf |= value << (32 - bit_left);
    if (n >= bit_left) {
        if (3 < s->buf_end - bs->buf_ptr) {
            AV_WL32(bs->buf_ptr, bit_buf);
            bs->buf_ptr += 4;
        } else {
            av_log(NULL, AV_LOG_ERROR, "Internal error, put_bits buffer too small\n");
            av_assert2(0);
        }
        bit_buf     = value >> bit_left;
        bit_left   += 32;
    }
    bit_left -= n;
#else
    if (n < bit_left) {
        bit_buf     = (bit_buf << n) | value;
        bit_left   -= n;
    } else {
        bit_buf   <<= bit_left;
        bit_buf    |= value >> (n - bit_left);
        if (3 < bs->buf_end - bs->buf_ptr) {
            AV_WB32(bs->buf_ptr, bit_buf);
            bs->buf_ptr += 4;
        } else {
            av_log(NULL, AV_LOG_ERROR, "Internal error, put_bits buffer too small\n");
            av_assert2(0);
        }
        bit_left   += 32 - n;
        bit_buf     = value;
    }
#endif

    bs->bit_buf  = bit_buf;
    bs->bit_left = bit_left;

}


static inline int bs_tell( bit_stream_t *bs )
{
    av_assert2( bs );
    av_assert2( bs->buf );
    av_assert2( bs->buf_ptr );
    return (bs->buf_ptr - bs->buf) * 8 + 32 - bs->bit_left;
}

static inline uint8_t *bs_ptr( bit_stream_t *bs )
{
    return bs->buf_ptr;
}

static inline void bs_flush(bit_stream_t *bs)
{
#ifndef BITSTREAM_WRITER_LE
    if (bs->bit_left < 32)
        bs->bit_buf <<= bs->bit_left;
#endif
    while (bs->bit_left < 32) {
        av_assert0(bs->buf_ptr < bs->buf_end);
#ifdef BITSTREAM_WRITER_LE
        *bs->buf_ptr++ = bs->bit_buf;
        bs->bit_buf  >>= 8;
#else
        *bs->buf_ptr++ = bs->bit_buf >> 24;
        bs->bit_buf  <<= 8;
#endif
        bs->bit_left  += 8;
    }
    bs->bit_left = 32;
    bs->bit_buf  = 0;
}

static inline void bs_skip_bytes( bit_stream_t *bs, int n)
{
    av_assert2((bs_tell(bs) & 7) == 0);
    av_assert2(bs->bit_left == 32);
    av_assert0(n <= bs->buf_end - bs->buf_ptr);
    bs->buf_ptr += n;
}

static void bs_write32(bit_stream_t *bs, uint32_t value)
{
    unsigned int bit_buf;
    int bit_left;

    bit_buf  = bs->bit_buf;
    bit_left = bs->bit_left;

#ifdef BITSTREAM_WRITER_LE
    bit_buf |= value << (32 - bit_left);
    if (3 < bs->buf_end - bs->buf_ptr) {
        AV_WL32(bs->buf_ptr, bit_buf);
        bs->buf_ptr += 4;
    } else {
        av_log(NULL, AV_LOG_ERROR, "Internal error, put_bits buffer too small\n");
        av_assert2(0);
    }
    bit_buf     = (uint64_t)value >> bit_left;
#else
    bit_buf     = (uint64_t)bit_buf << bit_left;
    bit_buf    |= value >> (32 - bit_left);
    if (3 < bs->buf_end - bs->buf_ptr) {
        AV_WB32(bs->buf_ptr, bit_buf);
        bs->buf_ptr += 4;
    } else {
        av_log(NULL, AV_LOG_ERROR, "Internal error, put_bits buffer too small\n");
        av_assert2(0);
    }
    bit_buf     = value;
#endif

    bs->bit_buf  = bit_buf;
    bs->bit_left = bit_left;
}

static inline void bs_write64( bit_stream_t *bs, uint64_t value, int n)
{
    av_assert2((n == 64) || (n < 64 && value < (UINT64_C(1) << n)));

    if (n < 32)
        bs_write(bs, value, n);
    else if (n == 32)
        bs_write32(bs, value);
    else if (n < 64) {
        uint32_t lo = value & 0xffffffff;
        uint32_t hi = value >> 32;
#ifdef BITSTREAM_WRITER_LE
        bs_write32(bs, lo);
        bs_write(bs, hi, n - 32);
#else
        bs_write(bs, hi, n - 32);
        bs_write32(bs, lo);
#endif
    } else {
        uint32_t lo = value & 0xffffffff;
        uint32_t hi = value >> 32;
#ifdef BITSTREAM_WRITER_LE
        bs_write32(bs, lo);
        bs_write32(bs, hi);
#else
        bs_write32(bs, hi);
        bs_write32(bs, lo);
#endif

    }
}

#endif  /*BITSTREAM_H*/

