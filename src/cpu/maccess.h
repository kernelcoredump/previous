 /*
  * UAE - The Un*x Amiga Emulator - CPU core
  *
  * Big endian memory access functions.
  *
  * Copyright 1996 Bernd Schmidt
  *
  * Adaptation to Hatari by Thomas Huth, Eero Tamminen
  *
  * This file is distributed under the GNU Public License, version 2 or at
  * your option any later version. Read the file gpl.txt for details.
  */

#ifndef UAE_MACCESS_H
#define UAE_MACCESS_H

#include <SDL_endian.h>

#if (SDL_BYTE_ORDER == SDL_BIG_ENDIAN)
#define do_get_mem_word(a) (*((uae_u16*)a))
#define do_put_mem_word(a, v) (*((uae_u16*)a)) = v
#define do_get_mem_long(a) (*((uae_u32*)a))
#define do_put_mem_long(a, v) (*((uae_u32*)a)) = v
#else
#ifdef _WIN32
#define do_get_mem_word(a) _byteswap_ushort(*((uae_u16 *)(a)))
#define do_put_mem_word(a, v) *((uae_u16 *)(a)) = _byteswap_ushort(v) 
#define do_get_mem_long(a) _byteswap_ulong(*((uae_u32 *)(a)))
#define do_put_mem_long(a, v) *((uae_u32 *)(a)) = _byteswap_ulong(v)
#else
#define do_get_mem_word(a) __builtin_bswap16(*((uae_u16 *)(a)))
#define do_put_mem_word(a, v) *((uae_u16 *)(a)) = __builtin_bswap16(v) 
#define do_get_mem_long(a) __builtin_bswap32(*((uae_u32 *)(a)))
#define do_put_mem_long(a, v) *((uae_u32 *)(a)) = __builtin_bswap32(v)
#endif
#endif

#define do_get_mem_byte(a) (*((uae_u8*)a))
#define do_put_mem_byte(a, v) (*((uae_u8*)a)) = v

#endif /* UAE_MACCESS_H */
