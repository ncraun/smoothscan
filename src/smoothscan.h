/*
  This file is part of smoothscan.

  smoothscan is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  smoothscan is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with smoothscan. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SMOOTHSCAN_H_INCLUDED
#define SMOOTHSCAN_H_INCLUDED

/* Change this for new releases */
#define SMOOTHSCAN_VERSION "0.1.0"

/* Represent the mapping from a symbol to a font code point */
struct mapping
{
  l_int32 iclass;		/* The symbol's index */
  unsigned char code_point;	/* The font code point */
  unsigned int font_num;	/* which font it belongs to */
  int used;			/* 1 if used, 0 if empty */
};

/* Hold the command line arguments for the program */
struct args
{
  /* Parameters */
  int num_input_files;
  char **input_files;
  char *outname;

  /* Optional Parameters */
  double thresh;
  double weight;

  /* Flags */
  int help_flag;
  int version_flag;

  /* Debug params */
  char *debug_tmpdir;
  /* Debug Flags */
  int debug_draw_borders;
  int debug_render_pages;
  int debug_skip_font_gen;
  int debug_no_clean_tmpdir;
};

/*
  Font segmentation:

  There is a limited number of codepoints available in the font, and
  we will likely need more than one font to include all the
  characters.

  Encoding used is KOI8-R, because of all the (non unicode) encodings
  supported by libharu and fontforge, KOI8-R allows for the most
  printable codepoints.

  With KOI8-R, we can encode from [33 (exclamation point) to 126
  (tilde)], [128 to 153], and [155 to 255]

  We have a total of 221 useable codepoints in KOI8-R
*/

/*
  Return the first valid code point of the font.
*/
unsigned char first_code_point ();

/*
  Return the last valid code point of the font.
 */
unsigned char max_code_point ();

/* 
   Return the next font code point after prev.
   
   next_code_point will NOT wrap around, you have to check against
   max_code_point yourself. However it will skip the nonprintable
   chars.
*/
unsigned char next_code_point (unsigned char prev);

/* 
   Print the text that is displayed when the user passes --help, and
   exit.
 */
void print_help ();

/*
  Print the text that is displayed when the user passes --version, and
  exit.
 */
void print_version ();

/*
  Prints str to stderr, and terminates the program.
 */
void error_quit (const char *str);

/*
  Return the number of digits that would be printed if a was
  represented in decimal form. Not valid for negative numbers.
 */
int num_digits (int a);

/*
  Callback for nftw to delete a file or a directory.
 */
int
delete_file (const char *path, const struct stat *sb, int typeflag,
	     struct FTW *ftwbuf);

/*
  Error handler callback for libharu. Simply prints libharu's
  generated error, and then error_quits itself.
 */
void
pdf_error_handler (HPDF_STATUS error_no, HPDF_STATUS detail_no,
		   void *user_data);

/*
  Out of memory errors should be considered as fatal errors for this
  program. This function is a wrapper around malloc that will check
  the malloc call for problems, and close the program if there is an
  out of memory error. Use this call instead of plain malloc.
*/
void *malloc_guarded (size_t size);

/*
  See if filename exists, and if we can read from it.

  Return 1 if it exists, 0 if it doesn't.
*/
int file_exists (const char *filename);

/*
  Create a font from the temp font directory with all the glyph image
  in it. Currently this just invoke the python font generation. 

  dirname - The directory the font is stored in.

  fontname - The output filename for the font, including the .ttf
  suffix

  latticeh, latticew - values from JBDATA

  fontnum - The internal number of the font (from the for loop).
 */
void
create_font_from_dir (const char *dirname, const char *fontname, int latticeh,
		      int latticew, int fontnum);

/*
  Generate the fonts that will be embedded in the output pdf.

  data - The JBDATA from leptonica's classifier.

  maps - Mapping each symbol to a font code point.

  num_fonts - The number of fonts to generate (calculated from the
  mapping phase)

  dir - The alternate directory to store files in. If NULL, then
  generate_fonts will use the value stored in the environmental var
  TMPDIR, if TMPDIR is empty it will use the value from POSIX's
  P_tmpdir, which should be something like /tmp or /var/tmp depending
  on your system.
 */
char *generate_fonts (const JBDATA * data, const struct mapping *maps,
		      int num_fonts, char *dir);

/*
  Create the pdf using libharu.

  outname - The filename to store the pdf into.

  tmpdirname - The temp directory where all the fonts are stored.

  num_fonts - The number of fonts.

  num_input_files - The number of input files (the number of pages in
  the pdf)

  data - JBDATA from leptonica
  
  maps - mappings from each symbol to its font code point

  debug_draw_borders - if 1, draw red rectangles where each glyph
  should be placed, if 0 don't.

*/
void
generate_pdf (const char *outname, const char *tmpdirname, int num_fonts,
	      int num_input_files, const JBDATA * data,
	      const struct mapping *maps, int debug_draw_borders);

/*
  Use leptonica to create the JBDATA, which is the dictionary of all
  the different symbols in the document.

  input_files - The list of 1bpp images (1 image per page).
  
  thresh - Specify the threshold value (value for correlation). Valid
  input is from [0.40 - 0.98]. Recommended values for scanned text
  from [0.80 - 0.85].  Default is 0.85.

  weight - Specify the weight value (correcting threshold for thick
  characters).  Valid input is from [0.0 - 1.0].  Recommended values
  for scanned text from [0.5 - 0.6].  Default is 0.5.
*/
JBDATA *classify_components (int num_input_files, char **input_files,
			     double thresh, double weight);


/*
  Map each symbol to a code point in the font.

  Returns the number of fonts needed.

  data - The leptonica JBDATA dictionary.

  in_maps - This is actually an output variable, it will be modified
  to hold all the generate mappings. It will be allocated in
  register_mappings, so it's up to the caller to free it.
*/
int register_mappings (const JBDATA * data, struct mapping **in_maps);

/*
  Create an arg struct with default parameters, and change them from
  the default according to the command line args. Uses
  getopt_long. Pass it the argc and argv from main.
*/
struct args *parse_args (int argc, char *argv[]);

/*
  Make sure all the command line arguments are valid.

  Return 1 if valid, error_quit if invalid.
*/
int validate_args (const struct args *args);

#endif /* SMOOTHSCAN_H_INCLUDED */
