/*******************************************************************************
 * Size: 16 px
 * Bpp: 1
 * Opts: --bpp 1 --size 16 --font C:/Users/zmm04/SquareLine/assets/name.ttf -o C:/Users/zmm04/SquareLine/assets\ui_font_name.c --format lvgl -r 0x20-0x7f --symbols °_- --no-compress --no-prefilter
 ******************************************************************************/

#include "ui.h"

#ifndef UI_FONT_NAME
#define UI_FONT_NAME 1
#endif

#if UI_FONT_NAME

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0xff, 0xf3, 0xc0,

    /* U+0022 "\"" */
    0xb6, 0xd0,

    /* U+0023 "#" */
    0x6c, 0xdb, 0xfb, 0x66, 0xcd, 0xbf, 0xb6, 0x6c,

    /* U+0024 "$" */
    0x23, 0xb7, 0x8c, 0x38, 0x63, 0xdb, 0x88,

    /* U+0025 "%" */
    0xde, 0xc6, 0x67, 0x33, 0x1b, 0xd8,

    /* U+0026 "&" */
    0x73, 0x6d, 0x9c, 0xdb, 0x66, 0xc0,

    /* U+0027 "'" */
    0xfe,

    /* U+0028 "(" */
    0x2f, 0x6d, 0xb6, 0xcc, 0x80,

    /* U+0029 ")" */
    0x99, 0xb6, 0xdb, 0x7a, 0x0,

    /* U+002A "*" */
    0x5d, 0x50,

    /* U+002B "+" */
    0x21, 0x3e, 0x42, 0x0,

    /* U+002C "," */
    0xf6,

    /* U+002D "-" */
    0xf0,

    /* U+002E "." */
    0xf0,

    /* U+002F "/" */
    0x18, 0xc6, 0x67, 0x33, 0x18, 0xc0,

    /* U+0030 "0" */
    0x76, 0xf7, 0xbd, 0xef, 0x7b, 0x70,

    /* U+0031 "1" */
    0x6e, 0x66, 0x66, 0x66, 0xf0,

    /* U+0032 "2" */
    0x76, 0xc6, 0x33, 0xbb, 0x98, 0xf8,

    /* U+0033 "3" */
    0x74, 0xc6, 0x37, 0xc, 0x73, 0x70,

    /* U+0034 "4" */
    0x18, 0xe3, 0x96, 0xdb, 0x6f, 0xc6, 0x18,

    /* U+0035 "5" */
    0xfe, 0x31, 0xed, 0x8c, 0x7b, 0x70,

    /* U+0036 "6" */
    0x19, 0x99, 0xed, 0xef, 0x7b, 0x70,

    /* U+0037 "7" */
    0xf8, 0xc6, 0x63, 0x19, 0x8c, 0x60,

    /* U+0038 "8" */
    0x76, 0xf7, 0xb7, 0x6f, 0x7b, 0x70,

    /* U+0039 "9" */
    0x76, 0xf7, 0xbd, 0xbc, 0xcc, 0xc0,

    /* U+003A ":" */
    0xf0, 0x3c,

    /* U+003B ";" */
    0xf0, 0x3d, 0x80,

    /* U+003C "<" */
    0x36, 0xc6, 0x30,

    /* U+003D "=" */
    0xf0, 0xf0,

    /* U+003E ">" */
    0xc6, 0x36, 0xc0,

    /* U+003F "?" */
    0x76, 0xf6, 0x37, 0x30, 0xc, 0x60,

    /* U+0040 "@" */
    0x7b, 0x3c, 0xf7, 0xdf, 0x7d, 0xb0, 0xc5, 0xe0,

    /* U+0041 "A" */
    0x76, 0xf7, 0xbd, 0xff, 0x7b, 0xd8,

    /* U+0042 "B" */
    0xf6, 0xf7, 0xbf, 0x6f, 0x7b, 0xf0,

    /* U+0043 "C" */
    0x76, 0xf1, 0x8c, 0x63, 0x1b, 0x70,

    /* U+0044 "D" */
    0xf6, 0xf7, 0xbd, 0xef, 0x7b, 0xf0,

    /* U+0045 "E" */
    0xfe, 0x31, 0x8f, 0xe3, 0x18, 0xf8,

    /* U+0046 "F" */
    0xfe, 0x31, 0x8f, 0xe3, 0x18, 0xc0,

    /* U+0047 "G" */
    0x76, 0xf1, 0x8f, 0xef, 0x7b, 0x78,

    /* U+0048 "H" */
    0xde, 0xf7, 0xbf, 0xef, 0x7b, 0xd8,

    /* U+0049 "I" */
    0xf6, 0x66, 0x66, 0x66, 0xf0,

    /* U+004A "J" */
    0x78, 0xc6, 0x31, 0x8c, 0x7b, 0x70,

    /* U+004B "K" */
    0xde, 0xf7, 0xbf, 0x6f, 0x7b, 0xd8,

    /* U+004C "L" */
    0xc6, 0x31, 0x8c, 0x63, 0x18, 0xf8,

    /* U+004D "M" */
    0xe7, 0xff, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb,
    0xdb,

    /* U+004E "N" */
    0xcf, 0x3e, 0xfb, 0xdf, 0x7c, 0xf3, 0xcc,

    /* U+004F "O" */
    0x76, 0xf7, 0xbd, 0xef, 0x7b, 0x70,

    /* U+0050 "P" */
    0xf6, 0xf7, 0xbf, 0x63, 0x18, 0xc0,

    /* U+0051 "Q" */
    0x76, 0xf7, 0xbd, 0xef, 0x7b, 0x71, 0x86,

    /* U+0052 "R" */
    0xf6, 0xf7, 0xbf, 0x6f, 0x7b, 0xd8,

    /* U+0053 "S" */
    0x76, 0xf1, 0x87, 0xc, 0x7b, 0x70,

    /* U+0054 "T" */
    0xfc, 0xc3, 0xc, 0x30, 0xc3, 0xc, 0x30,

    /* U+0055 "U" */
    0xde, 0xf7, 0xbd, 0xef, 0x7b, 0x70,

    /* U+0056 "V" */
    0xde, 0xf7, 0xbd, 0xed, 0xce, 0x20,

    /* U+0057 "W" */
    0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0x7e, 0x7e,
    0x24,

    /* U+0058 "X" */
    0xde, 0xf7, 0xb7, 0x6f, 0x7b, 0xd8,

    /* U+0059 "Y" */
    0xcf, 0x3c, 0xf3, 0x78, 0xc3, 0xc, 0x30,

    /* U+005A "Z" */
    0xf8, 0xc6, 0x77, 0x73, 0x18, 0xf8,

    /* U+005B "[" */
    0xfb, 0x6d, 0xb6, 0xdb, 0x80,

    /* U+005C "\\" */
    0xc6, 0x30, 0xc7, 0x18, 0x63, 0x18,

    /* U+005D "]" */
    0xed, 0xb6, 0xdb, 0x6f, 0x80,

    /* U+005E "^" */
    0x31, 0xec, 0xf3,

    /* U+005F "_" */
    0xf0,

    /* U+0060 "`" */
    0x90,

    /* U+0061 "a" */
    0x76, 0xc6, 0xfd, 0xed, 0xe0,

    /* U+0062 "b" */
    0xc6, 0x3d, 0xbd, 0xef, 0x7b, 0xf0,

    /* U+0063 "c" */
    0x76, 0xf1, 0x8c, 0x6d, 0xc0,

    /* U+0064 "d" */
    0x18, 0xdf, 0xbd, 0xef, 0x7b, 0x78,

    /* U+0065 "e" */
    0x76, 0xf7, 0xfc, 0x6d, 0xc0,

    /* U+0066 "f" */
    0x36, 0xf6, 0x66, 0x66, 0x60,

    /* U+0067 "g" */
    0x7e, 0xf7, 0xbd, 0xed, 0xe3, 0xf0,

    /* U+0068 "h" */
    0xc6, 0x3d, 0xbd, 0xef, 0x7b, 0xd8,

    /* U+0069 "i" */
    0x60, 0xe6, 0x66, 0x66, 0xf0,

    /* U+006A "j" */
    0x63, 0xb6, 0xdb, 0x6f, 0x0,

    /* U+006B "k" */
    0xc6, 0x37, 0xbd, 0xfb, 0x7b, 0xd8,

    /* U+006C "l" */
    0xff, 0xff, 0xc0,

    /* U+006D "m" */
    0xfe, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb,

    /* U+006E "n" */
    0xf6, 0xf7, 0xbd, 0xef, 0x60,

    /* U+006F "o" */
    0x76, 0xf7, 0xbd, 0xed, 0xc0,

    /* U+0070 "p" */
    0xf6, 0xf7, 0xbd, 0xef, 0xd8, 0xc0,

    /* U+0071 "q" */
    0x7e, 0xf7, 0xbd, 0xed, 0xe3, 0x18,

    /* U+0072 "r" */
    0xf6, 0xf1, 0x8c, 0x63, 0x0,

    /* U+0073 "s" */
    0x76, 0xf0, 0xe1, 0xed, 0xc0,

    /* U+0074 "t" */
    0x66, 0xf6, 0x66, 0x66, 0x30,

    /* U+0075 "u" */
    0xde, 0xf7, 0xbd, 0xed, 0xe0,

    /* U+0076 "v" */
    0xde, 0xf7, 0xbd, 0xb8, 0x80,

    /* U+0077 "w" */
    0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0x7e, 0x24,

    /* U+0078 "x" */
    0xde, 0xf6, 0xed, 0xef, 0x60,

    /* U+0079 "y" */
    0xde, 0xf7, 0xbd, 0xed, 0xe3, 0xf0,

    /* U+007A "z" */
    0xf8, 0xce, 0xee, 0x63, 0xe0,

    /* U+007B "{" */
    0x36, 0x66, 0x6c, 0x66, 0x66, 0x30,

    /* U+007C "|" */
    0xff, 0xff, 0xfc,

    /* U+007D "}" */
    0xc6, 0x66, 0x63, 0x66, 0x66, 0xc0,

    /* U+007E "~" */
    0x3f, 0xc0,

    /* U+00B0 "°" */
    0x76, 0xf7, 0xb7, 0x0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 48, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 48, .box_w = 2, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 4, .adv_w = 64, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 6, .adv_w = 128, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 14, .adv_w = 96, .box_w = 5, .box_h = 11, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 21, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 27, .adv_w = 112, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 33, .adv_w = 48, .box_w = 2, .box_h = 4, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 34, .adv_w = 64, .box_w = 3, .box_h = 11, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 39, .adv_w = 64, .box_w = 3, .box_h = 11, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 44, .adv_w = 64, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 46, .adv_w = 96, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 50, .adv_w = 48, .box_w = 2, .box_h = 4, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 51, .adv_w = 80, .box_w = 4, .box_h = 1, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 52, .adv_w = 48, .box_w = 2, .box_h = 2, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 53, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 59, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 65, .adv_w = 80, .box_w = 4, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 70, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 76, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 82, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 89, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 95, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 101, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 107, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 113, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 119, .adv_w = 48, .box_w = 2, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 121, .adv_w = 48, .box_w = 2, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 124, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 127, .adv_w = 80, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 129, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 132, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 138, .adv_w = 112, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 146, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 152, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 158, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 164, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 170, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 176, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 182, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 188, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 194, .adv_w = 80, .box_w = 4, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 199, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 205, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 211, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 217, .adv_w = 144, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 226, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 233, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 239, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 245, .adv_w = 96, .box_w = 5, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 252, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 258, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 264, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 271, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 277, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 283, .adv_w = 144, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 292, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 298, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 305, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 311, .adv_w = 64, .box_w = 3, .box_h = 11, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 316, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 322, .adv_w = 64, .box_w = 3, .box_h = 11, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 327, .adv_w = 112, .box_w = 6, .box_h = 4, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 330, .adv_w = 80, .box_w = 4, .box_h = 1, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 331, .adv_w = 48, .box_w = 2, .box_h = 2, .ofs_x = 0, .ofs_y = 9},
    {.bitmap_index = 332, .adv_w = 96, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 337, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 343, .adv_w = 96, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 348, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 354, .adv_w = 96, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 359, .adv_w = 80, .box_w = 4, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 364, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 370, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 376, .adv_w = 80, .box_w = 4, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 381, .adv_w = 64, .box_w = 3, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 386, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 392, .adv_w = 48, .box_w = 2, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 395, .adv_w = 144, .box_w = 8, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 402, .adv_w = 96, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 407, .adv_w = 96, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 412, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 418, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 424, .adv_w = 96, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 429, .adv_w = 96, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 434, .adv_w = 80, .box_w = 4, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 439, .adv_w = 96, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 444, .adv_w = 96, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 449, .adv_w = 144, .box_w = 8, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 456, .adv_w = 96, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 461, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 467, .adv_w = 96, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 472, .adv_w = 80, .box_w = 4, .box_h = 11, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 478, .adv_w = 48, .box_w = 2, .box_h = 11, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 481, .adv_w = 80, .box_w = 4, .box_h = 11, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 487, .adv_w = 80, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 489, .adv_w = 96, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 4}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 95, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 176, .range_length = 1, .glyph_id_start = 96,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 2,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t ui_font_name = {
#else
lv_font_t ui_font_name = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 13,          /*The maximum line height required by the font*/
    .base_line = 2,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -2,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if UI_FONT_NAME*/

