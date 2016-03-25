#ifndef CORE_CPP_GRAPHICS_H
#define CORE_CPP_GRAPHICS_H


#include <string>

namespace corecpp
{

/* select graphic rendition parameters */
enum struct sgr_p : int
{
	all_off =  0,
	bold 	=  1,
	faint 	=  2,
	italic 	=  3,
	underline 	=  4,
	blink 	=  5,
	fast_blink 	=  6,
	negative 	=  7,
	conceal 	=  8,
	crossed_out 	= 9,
	font_primary 	= 10,
	font_alt_1 	= 11,
	font_alt_2 	= 12,
	font_alt_3 	= 13,
	font_alt_4 	= 14,
	font_alt_5 	= 15,
	font_alt_6 	= 16,
	font_alt_7 	= 17,
	font_alt_8 	= 18,
	font_alt_9 	= 19,
	fraktur 	= 20, /* hardly ever supported */
	bold_off 	= 21, /* or Underline: Double, none widely supported */
	normal 		= 22, /* Neither bold nor faint */
	italic_off	= 23, /* Not italic, not Fraktur */
	no_underline	= 24, /* Not italic, not Fraktur */
	no_blink	= 25, /* Not italic, not Fraktur */
	/* 26 is reserved */
	positive	= 27, /* Not italic, not Fraktur */
	reveal		= 28, /* conceal off */
	no_cross	= 29, /* no crossed_out */
	fg_black	= 30,
	fg_red	= 31,
	fg_green	= 32,
	fg_yellow	= 33,
	fg_blue	= 34,
	fg_magenta	= 35,
	fg_cyan	= 36,
	fg_white	= 37,
	/* 38 is reserved */
	fg_default	= 39,
	bg_black	= 40,
	bg_red	= 41,
	bg_green	= 42,
	bg_yellow	= 43,
	bg_blue	= 44,
	bg_magenta	= 45,
	bg_cyan	= 46,
	bg_white	= 47,
	/* 48 is reserved */
	bg_default	= 49,
	/* 50 is reserved */
	framed	= 49,
	encircled	= 49,
	overlined	= 49,
	no_counter	= 49, /* Not framed or encircled */
	no_overline	= 55,
	/* 56 to 59 are reserved */
	ideogram_underline	= 60,
	ideogram_double_underline	= 61,
	ideogram_overline	= 62,
	ideogram_double_overline	= 63,
	ideogram_stress		= 64,

	/* aixterm (not in standard) */
	fg_black_high	= 90,
	fg_red_high	= 91,
	fg_green_high	= 92,
	fg_yellow_high	= 93,
	fg_blue_high	= 94,
	fg_magenta_high	= 95,
	fg_cyan_high	= 96,
	fg_white_high	= 97,
	bg_black_high	= 100,
	bg_red_high	= 101,
	bg_green_high	= 102,
	bg_yellow_high	= 103,
	bg_blue_high	= 104,
	bg_magenta_high	= 105,
	bg_cyan_high	= 106,
	bg_white_high	= 107
};


static constexpr const char* rendition_strs[40] = {
	"0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
	"10", "11", "12", "13", "14", "15", "16", "17", "18", "19",
	"20", "21", "22", "23", "24", "25", "26", "27", "28", "29",
	"30", "31", "32", "33", "34", "35", "36", "37", "38", "39"
};
constexpr const char* to_char(sgr_p r)
{
	return rendition_strs[(int)r];
}

template <sgr_p param>
struct graphic_rendition
{
	static const std::string value;
};

template <sgr_p param>
const std::string graphic_rendition<param>::value = std::string("\x1b[") + to_char(param) + "m";

#if defined __cplusplus && __cplusplus > 201103L
template <sgr_p param>
const std::string graphic_rendition_v = graphic_rendition<param>::value;
#endif

}

#endif
