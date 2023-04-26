/* Conversion from UTF-16 to legacy encodings.
   Copyright (C) 2002, 2006-2022 Free Software Foundation, Inc.

   This file is free software.
   It is dual-licensed under "the GNU LGPLv3+ or the GNU GPLv2+".
   You can redistribute it and/or modify it under either
     - the terms of the GNU Lesser General Public License as published
       by the Free Software Foundation, either version 3, or (at your
       option) any later version, or
     - the terms of the GNU General Public License as published by the
       Free Software Foundation; either version 2, or (at your option)
       any later version, or
     - the same dual license "the GNU LGPLv3+ or the GNU GPLv2+".

   This file is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License and the GNU General Public License
   for more details.

   You should have received a copy of the GNU Lesser General Public
   License and of the GNU General Public License along with this
   program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible <bruno@clisp.org>.  */

#include <config.h>

/* Specification.  */
#include "uniconv.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "striconveha.h"
#include "unistr.h"

#define SIZEOF(array) (sizeof (array) / sizeof (array[0]))

/* Name of UTF-16 encoding with machine dependent endianness and alignment.  */
#if defined _LIBICONV_VERSION || (((__GLIBC__ > 2) || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 2)) && !defined __UCLIBC__)
# ifdef WORDS_BIGENDIAN
#  define UTF16_NAME "UTF-16BE"
# else
#  define UTF16_NAME "UTF-16LE"
# endif
#endif


#if !defined UTF16_NAME

/* A variant of u16_to_u8 that treats an incomplete sequence of units at the
   end as a harmless no-op, rather than reporting it as an EILSEQ error.  */

#define FUNC u16_to_u8_lenient
#define SRC_UNIT uint16_t
#define DST_UNIT uint8_t

static DST_UNIT *
FUNC (const SRC_UNIT *s, size_t n, DST_UNIT *resultbuf, size_t *lengthp)
{
  const SRC_UNIT *s_end = s + n;
  /* Output string accumulator.  */
  DST_UNIT *result;
  size_t allocated;
  size_t length;

  if (resultbuf != NULL)
    {
      result = resultbuf;
      allocated = *lengthp;
    }
  else
    {
      result = NULL;
      allocated = 0;
    }
  length = 0;
  /* Invariants:
     result is either == resultbuf or == NULL or malloc-allocated.
     If length > 0, then result != NULL.  */

  while (s < s_end)
    {
      ucs4_t uc;
      int count;

      /* Fetch a Unicode character from the input string.  */
      count = u16_mbtoucr (&uc, s, s_end - s);
      if (count < 0)
        {
          if (count == -2)
            /* Incomplete sequence of units.  */
            break;
          if (!(result == resultbuf || result == NULL))
            free (result);
          errno = EILSEQ;
          return NULL;
        }
      s += count;

      /* Store it in the output string.  */
      count = u8_uctomb (result + length, uc, allocated - length);
      if (count == -1)
        {
          if (!(result == resultbuf || result == NULL))
            free (result);
          errno = EILSEQ;
          return NULL;
        }
      if (count == -2)
        {
          DST_UNIT *memory;

          allocated = (allocated > 0 ? 2 * allocated : 12);
          if (length + 6 > allocated)
            allocated = length + 6;
          if (result == resultbuf || result == NULL)
            memory = (DST_UNIT *) malloc (allocated * sizeof (DST_UNIT));
          else
            memory =
              (DST_UNIT *) realloc (result, allocated * sizeof (DST_UNIT));

          if (memory == NULL)
            {
              if (!(result == resultbuf || result == NULL))
                free (result);
              errno = ENOMEM;
              return NULL;
            }
          if (result == resultbuf && length > 0)
            memcpy ((char *) memory, (char *) result,
                    length * sizeof (DST_UNIT));
          result = memory;
          count = u8_uctomb (result + length, uc, allocated - length);
          if (count < 0)
            abort ();
        }
      length += count;
    }

  if (length == 0)
    {
      if (result == NULL)
        {
          /* Return a non-NULL value.  NULL means error.  */
          result = (DST_UNIT *) malloc (1);
          if (result == NULL)
            {
              errno = ENOMEM;
              return NULL;
            }
        }
    }
  else if (result != resultbuf && length < allocated)
    {
      /* Shrink the allocated memory if possible.  */
      DST_UNIT *memory;

      memory = (DST_UNIT *) realloc (result, length * sizeof (DST_UNIT));
      if (memory != NULL)
        result = memory;
    }

  *lengthp = length;
  return result;
}

#undef DST_UNIT
#undef SRC_UNIT
#undef FUNC

#endif


#define FUNC u16_conv_to_encoding
#define UNIT uint16_t
#define U_TO_U8 u16_to_u8_lenient
#define U_MBLEN u16_mblen
#if defined UTF16_NAME
# define UTF_NAME UTF16_NAME
# define HAVE_UTF_NAME 1
#endif
#include "u-conv-to-enc.h"
