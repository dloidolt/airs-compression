/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Big-Endian Bitstream Writer
 *
 * Usage:
 * - Initialize the bitstream writer:
 *        error_code = bitstream_write_init();
 * - Write bits to the bitstream:
 *        error_code = bitstream_write();
 * - Flush remaining bits to the buffer:
 *        bytes_written = bitstream_flush();
 */

#ifndef CMP_BITSTREAM_WRITER_H
#define CMP_BITSTREAM_WRITER_H

#include <stdint.h>
#include <stddef.h>

#include "../common/err_private.h"
#include "../common/compiler.h"


/**
 * @brief This structure maintains the state of the bitstream writer
 *
 * @warning This structure MUST NOT be directly manipulated by external code.
 *	Always use the provided API functions to interact with the structure.
 */

struct bitstream_writer {
	uint64_t cache;	      /**< Local bit cache  */
	unsigned int bit_cap; /**< Bit capacity left in the cache */
	uint8_t *start;	      /**< Beginning of bitstream */
	uint8_t *ptr;	      /**< Current write position */
	uint8_t *end;	      /**< End of the bitstream pointer */
	int overflow;	      /**< Set to non-zero if buffer overflows */
};


/**
 * @brief Initializes a bitstream writer
 *
 * @param bs	pointer to an already allocated bitstream_writer structure
 * @param dst	start address of the bitstream buffer; has to be 8-byte aligned
 * @param size	capacity of the bitstream buffer in bytes
 *
 * @returns an error code, which can be checked using cmp_is_error()
 */

static __inline uint32_t bitstream_writer_init(struct bitstream_writer *bs, void *dst,
					       uint32_t size)
{
	if (!bs)
		return CMP_ERROR(INT_BITSTREAM);
	if (!dst)
		return CMP_ERROR(DST_NULL);
	if ((uintptr_t)dst & 7)
		return CMP_ERROR(DST_UNALIGNED);

	bs->cache = 0;
	bs->bit_cap = 64;
	bs->start = dst;
	bs->ptr = dst;
	bs->end = (uint8_t *)dst + size;
	bs->overflow = 0;

	return CMP_ERROR(NO_ERROR);
}


/**
 * @brief Stores a 64-bit integer as big-endian bytes
 */

static __inline void put_be64(void *ptr, uint64_t val)
{
	uint8_t *p = ptr;

	p[0] = (uint8_t)(val >> 56);
	p[1] = (uint8_t)(val >> 48);
	p[2] = (uint8_t)(val >> 40);
	p[3] = (uint8_t)(val >> 32);
	p[4] = (uint8_t)(val >> 24);
	p[5] = (uint8_t)(val >> 16);
	p[6] = (uint8_t)(val >> 8);
	p[7] = (uint8_t)val;
}


/**
 * @brief Write up to 32 bits to the bitstream
 *
 * @param bs		pointer to a initialized bitstream_writer structure
 * @param value		bits to write to the bitstream
 * @param nb_bits	number of bits to write from value
 *
 * @returns an error code, which can be checked using cmp_is_error()
 */

static __inline uint32_t bitstream_write32(struct bitstream_writer *bs, uint32_t value,
					   unsigned int nb_bits)
{
	if (!bs)
		return CMP_ERROR(INT_BITSTREAM);
	if (nb_bits > 32)
		return CMP_ERROR(INT_BITSTREAM);
	if (nb_bits < 32 && (value >> nb_bits))
		return CMP_ERROR(INT_BITSTREAM);
	if (bs->overflow)
		return CMP_ERROR(DST_TOO_SMALL);

	/* Fast path: bits fit in current cache */
	if (nb_bits < bs->bit_cap) {
		bs->cache = (bs->cache << nb_bits) | value;
		bs->bit_cap -= nb_bits;
		return CMP_ERROR(NO_ERROR);
	}

	/* Slow path: need to flush cache */
	if (bs->end - bs->ptr >= 8) {
		bs->cache <<= bs->bit_cap;
		bs->cache |= value >> (nb_bits - bs->bit_cap);
		put_be64(bs->ptr, bs->cache);

		bs->ptr += 8;
		bs->cache = value;
		bs->bit_cap += 64 - nb_bits;
		return CMP_ERROR(NO_ERROR);
	}

	bs->overflow = 1;
	return CMP_ERROR(DST_TOO_SMALL);
}


/**
 * @brief Write up to 64 bits to the bitstream
 *
 * @param bs		pointer to a initialized bitstream_writer structure
 * @param value		bits to write to the bitstream
 * @param nb_bits	number of bits to write from value
 *
 * @returns an error code, which can be checked using cmp_is_error()
 */

static __inline uint32_t bitstream_write64(struct bitstream_writer *bs, uint64_t value,
					   unsigned int nb_bits)
{
	uint32_t ret;

	if (nb_bits > 64)
		return CMP_ERROR(INT_BITSTREAM);

	if (nb_bits <= 32) {
		ret = bitstream_write32(bs, (uint32_t)value, nb_bits);
	} else {
		uint32_t hi = (uint32_t)(value >> 32);
		uint32_t lo = (uint32_t)value;

		ret = bitstream_write32(bs, hi, nb_bits - 32);
		if (cmp_is_error_int(ret))
			return ret;
		ret = bitstream_write32(bs, lo, 32);
	}
	return ret;
}


/**
 * @brief Flushes remaining bits form the internal cache to the buffer
 * Last byte may be padded with zeros
 *
 * @param bs	pointer to a initialized bitstream_writer structure
 *
 * @returns written bytes to bitstream or an error code, which can be checked
 *	using cmp_is_error()
 */

static __inline uint32_t bitstream_flush(struct bitstream_writer *bs)
{
	unsigned int bytes;
	uint8_t *cursor;

	if (!bs)
		return CMP_ERROR(INT_BITSTREAM);
	if (bs->overflow)
		return CMP_ERROR(DST_TOO_SMALL);

	cursor = bs->ptr;
	bytes = (64 - bs->bit_cap + 7) / 8;
	if (bytes) {
		uint64_t tmp = bs->cache << bs->bit_cap;

		while (bytes--) {
			if (cursor >= bs->end) {
				bs->overflow = 1;
				return CMP_ERROR(DST_TOO_SMALL);
			}
			*cursor++ = (uint8_t)(tmp >> (64 - 8));
			tmp <<= 8;
		}
	}

	return (uint32_t)(cursor - bs->start);
}


/**
 * @brief Calculates current total written size in bytes
 *
 * @param bs	pointer to the initialized bitstream_writer structure
 *
 * @returns total bytes effectively written including non-flushed cached bits or
 *	an error code, which can be checked using cmp_is_error()
 */

static __inline uint32_t bitstream_size(const struct bitstream_writer *bs)
{
	if (!bs)
		return CMP_ERROR(INT_BITSTREAM);
	if (bs->overflow)
		return CMP_ERROR(DST_TOO_SMALL);

	return (uint32_t)(bs->ptr - bs->start) + (64 - (uint32_t)bs->bit_cap + 7) / 8;
}


/**
 * @brief Reset the bitstream writer to the beginning of its buffer
 *
 * @param bs	pointer to the initialized bitstream_writer structure
 *
 * @returns an error code, which can be checked using cmp_is_error()
 */

static __inline uint32_t bitstream_rewind(struct bitstream_writer *bs)
{
	uint32_t ret;

	if (!bs)
		return CMP_ERROR(INT_BITSTREAM);
	if (bs->overflow)
		return CMP_ERROR(DST_TOO_SMALL);

	ret = bitstream_flush(bs);
	if (cmp_is_error_int(ret))
		return ret;

	return bitstream_writer_init(bs, bs->start, (uint32_t)(bs->end - bs->start));
}


#endif /* CMP_BITSTREAM_WRITER_H */
