/* palette_neon_intrinsics.c - NEON optimised palette expansion functions
 *
 * Copyright (c) 2017 ARM Limited
 * Copyright (c) 2017 Glenn Randers-Pehrson
 * Written by Richard Townsend <Richard.Townsend@arm.com>, February 2017.
 *
 * This code is released under the libpng license.
 * For conditions of distribution and use, see the disclaimer
 * and license in png.h
 */

#include "../pngpriv.h"

#if PNG_ARM_NEON_IMPLEMENTATION == 1

#include <arm_neon.h>

/* Build an RGBA palette from the RGB and separate alpha palettes. */
void
png_riffle_palette_rgba(png_structrp png_ptr, png_row_infop row_info)
{
   png_const_colorp palette = png_ptr->palette;
   png_bytep riffled_palette = png_ptr->row_tmp_palette;
   png_const_bytep trans_alpha = png_ptr->trans_alpha;
   int num_trans = png_ptr->num_trans;

   if (row_info->bit_depth != 8) {
      png_error(png_ptr, "bit_depth must be 8 for png_riffle_palette_rgba");
      return;
   }

   uint8x8x4_t w = {
      {0, 0, 0, 0, 0, 0, 0}
   };
   w.val[3] = vdup_n_u8(0xff);

   int i;
   /* First, riffle the RGB colours into a RGBA palette, the A value is
    * currently garbage. */
   for (i = 0; i < (1 << row_info->bit_depth); i += 8) {
      uint8x8x3_t v = vld3_u8((png_const_bytep)(palette + i));
      w.val[0] = v.val[0];
      w.val[1] = v.val[1];
      w.val[2] = v.val[2];
      vst4_u8(riffled_palette + (i << 2), w);
   }

   /* Next, fix up the garbage values. */
   for (i = 0; i < num_trans; i++) {
      riffled_palette[(i << 2) + 3] = trans_alpha[i];
   }
}


/* Expands a palettized row into RGBA. */
int
png_do_expand_palette_neon_rgba(png_structrp png_ptr, png_row_infop row_info,
   png_const_bytep row, const png_bytepp ssp, const png_bytepp ddp)
{

   png_uint_32 row_width = row_info->width;
   const png_uint_32 *riffled_palette = (const png_uint_32*)png_ptr->row_tmp_palette;

   if (row_width >= 4) {
      /* This function originally gets the last byte of the output row
         The NEON part writes forward from a given position, so we have
         to seek this back by 4 pixels x 4 bytes, for a total offset of 16.
         We should only update the output pointer if the loop's going to run. */
      *ddp = *ddp - 15;
   }

   int i;

   for(i = 0; i < row_width; i += 4) {
      uint32x4_t cur;
      png_bytep sp = *ssp - i, dp = *ddp - (i << 2);
      cur = vld1q_dup_u32 (riffled_palette + *(sp - 3));
      cur = vld1q_lane_u32(riffled_palette + *(sp - 2), cur, 1);
      cur = vld1q_lane_u32(riffled_palette + *(sp - 1), cur, 2);
      cur = vld1q_lane_u32(riffled_palette + *(sp), cur, 3);
      vst1q_u32((void *)dp, cur);
   }
   if (i != row_width) {
      i -= 4; /* Remove the amount that wasn't processed. */
   }

   /* Decrement output pointers. */
   *ssp = *ssp - i;
   *ddp = *ddp - (i << 2);
   return i;
}

/* Expands a palettized row into RGB format. */
int
png_do_expand_palette_neon_rgb(png_structrp png_ptr, png_row_infop row_info,
   png_const_bytep row, const png_bytepp ssp, const png_bytepp ddp)
{
   png_uint_32 row_width = row_info->width;
   png_const_bytep palette = (png_const_bytep)png_ptr->palette;

   int i;

   if (row_width >= 8) {
      /* Seeking this back by 8 pixels x 3 bytes. */
      *ddp = *ddp - 23;
   }

   for (i = 0; i < row_width; i += 8) {
      uint8x8x3_t cur;
      png_bytep sp = *ssp - i, dp = *ddp - ((i << 1) + i);
      cur = vld3_dup_u8(palette + sizeof(png_color) * (*(sp - 7)));
      cur = vld3_lane_u8(palette + sizeof(png_color) * (*(sp - 6)), cur, 1);
      cur = vld3_lane_u8(palette + sizeof(png_color) * (*(sp - 5)), cur, 2);
      cur = vld3_lane_u8(palette + sizeof(png_color) * (*(sp - 4)), cur, 3);
      cur = vld3_lane_u8(palette + sizeof(png_color) * (*(sp - 3)), cur, 4);
      cur = vld3_lane_u8(palette + sizeof(png_color) * (*(sp - 2)), cur, 5);
      cur = vld3_lane_u8(palette + sizeof(png_color) * (*(sp - 1)), cur, 6);
      cur = vld3_lane_u8(palette + sizeof(png_color) * (*(sp - 0)), cur, 7);
      vst3_u8((void *)dp, cur);
   }

   if (i != row_width) {
      i -= 3; /* Remove the amount that wasn't processed. */
   }

   /* Decrement ouput pointers. */
   *ssp = *ssp - i;
   *ddp = *ddp - ((i << 1) + i);
   return i;
}

#endif /* PNG_ARM_NEON_IMPLEMENTATION */
