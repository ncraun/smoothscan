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

#define _GNU_SOURCE

/* Standard headers */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

/* POSIX specific headers */
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <ftw.h>

/* GNU specific headers (Work on Windows?)*/
#include <getopt.h>

/* 3rd party library headers */
#include <leptonica/allheaders.h>
#include <hpdf.h>
/* #include <potracelib.h> */

#include "smoothscan.h"

int
main (int argc, char *argv[])
{
  struct args *args = parse_args (argc, argv);

  validate_args (args);

  JBDATA *data =
    classify_components (args->num_input_files, args->input_files,
			 args->thresh, args->weight);

  /* Render output of leptonica's classifier, if requested */
  if (args->debug_render_pages)
    {
      int i;
      PIXA *pa = jbDataRender (data, 0);
      for (i = 0; i < pa->n; i++)
	{
	  PIX *pix = pa->pix[i];
	  char filename[512];
	  sprintf (filename, "rendered_%05d.png", i);
	  pixWrite (filename, pix, IFF_PNG);
	}
    }

  struct mapping *maps = NULL;
  int num_fonts = register_mappings (data, &maps);

  char *tmpdirname = NULL;

  if (!args->debug_skip_font_gen)
    {
      tmpdirname = generate_fonts (data, maps, num_fonts, args->debug_tmpdir);
    }

  generate_pdf (args->outname, tmpdirname, num_fonts, args->num_input_files,
		data, maps, args->debug_draw_borders);

  /* clean up tmpdir */

  if (!args->debug_no_clean_tmpdir)
    {
      /* This may not be Windows compatible */
      if (nftw (tmpdirname, delete_file, 64, FTW_DEPTH | FTW_PHYS) == -1)
	{
	  error_quit ("Failed to clean up tmpdir.");
	}
    }

  if (args->debug_tmpdir == NULL)
    {
      free (tmpdirname);
    }

  free (maps);
  jbDataDestroy (&data);
  free (args->input_files);
  free (args);
  return 0;
}

unsigned char
first_code_point ()
{
  return 33;
}

unsigned char
max_code_point ()
{
  return 255;
}

unsigned char
next_code_point (unsigned char prev)
{
  /* 128 to 153 */
  if (prev == 126)
    return 128;
  /* 155 to 255 */
  if (prev == 153)
    return 155;

  return prev + 1;
}

void
print_help ()
{
  printf (
          "Usage: smoothscan [debug-options] [options] -o output.pdf inputs\n"
          "\n"
	  "Please read the man page for more in depth information.\n"
	  "inputs is the list of 1bpp TIFF files, one file per page\n"
          "\n"
	  "Regular Options:\n"
	  "    -o, --output FILE : Place the output into FILE.\n"
	  "    -t, --thresh VALUE\n"
	  "        Specify the threshold value [0.40 - 0.98], Default 0.85.\n"
	  "    -w, --weight VALUE\n"
	  "        Specify the weight value [0.0 - 1.0], Default 0.5.\n"
	  "    -h, --help\n"
	  "        Display basic usage information.\n"
	  "    -v, --version\n"
	  "        Display version information.\n"
          "\n"
	  "Debug options:\n"
	  "    --debug-tmpdir TMPDIR\n"
	  "        Use specified tmpdir instead of system tmpdir.\n"
	  "    --debug-draw-borders\n"
	  "        Draw red rectangles around each glyph in output pdf.\n"
	  "    --debug-render-pages\n"
	  "        Render output to image files in addition to pdf output.\n"
	  "    --debug-skip-font-gen\n"
	  "        Skip font generation step. Won't work if tmpdir doesn't already have fonts in it.\n"
	  "    --debug-no-clean-tmpdir\n"
	  "        Don't delete temporary files from tmpdir when processing is done.\n"
	  "\n"
	  "Report bugs to nate@natecraun.net or on the Github bug tracker\n"
	  "Smoothscan homepage: <https://natecraun.net/projects/smoothscan/>\n"
	  "Github Project page: <https://github.com/ncraun/smoothscan>\n"
          );
}

void
print_version ()
{
  printf ("Smoothscan " SMOOTHSCAN_VERSION "\n"
	  "Copyright (C) 2013 Nate Craun\n"
	  "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n"
	  "This is free software: you are free to change and redistribute it.\n"
	  "There is NO WARRANTY, to the extent permitted by law.\n");

}

void
error_quit (const char *str)
{
  fprintf (stderr, "Error: %s\nSystem Error: %s\n", str, strerror (errno));
  exit (0);
}

int
num_digits (int a)
{
  int n = 0;

  if (a == 0)
    return 1;

  while (a > 0)
    {
      a /= 10;
      n++;
    }

  return n;
}

int
delete_file (const char *path, const struct stat *sb, int typeflag,
	     struct FTW *ftwbuf)
{
  int ret = remove (path);

  if (ret == -1)
    {
      error_quit ("Could not remove temp file.");
    }

  return ret;
}

void
pdf_error_handler (HPDF_STATUS error_no, HPDF_STATUS detail_no,
		   void *user_data)
{
  fprintf (stderr, "libharu error: error_no=%04X, detail_no=%d\n",
	   (unsigned int) error_no, (int) detail_no);
  error_quit ("PDF Generation problem.");
}


void *
malloc_guarded (size_t size)
{
  void *vptr = malloc (size);

  if (vptr == NULL)
    {
      error_quit ("Out of memory.");
    }

  return vptr;
}

int
file_exists (const char *filename)
{
  return (access (filename, R_OK) != -1);
}


void
create_font_from_dir (const char *dirname, const char *fontname, int latticeh,
		      int latticew, int fontnum)
{
  /* Call python from here */
  int commandlen =
    strlen ("smoothscan-fontgen.py") + 1 + strlen (dirname) + 1 +
    strlen (fontname) + 1 + num_digits (latticeh) + 1 +
    num_digits (latticew) + 1 + num_digits (fontnum);

  char *command = malloc_guarded (commandlen + 1);
  sprintf (command, "smoothscan-fontgen.py %s %s %d %d %d", dirname,
	   fontname, latticeh, latticew, fontnum);
  system (command);
  free (command);
}


char*
generate_fonts (const JBDATA * data, const struct mapping *maps,
		int num_fonts, char *dir)
{
  int dirnamelen = 0;
  char *dirname = NULL;

  if (dir == NULL)
    {

      /* Create a file in the system temp dir (usually /tmp) */
      /* This may not be portable to Windows */
      char suffix[] = "smoothscan_XXXXXX";
      /* Use the value of environmental var TMPDIR if it is set */
      char *tmpdir = getenv ("TMPDIR");
      if (tmpdir == NULL)
	{
	  /* Load the default temp directory */
	  tmpdir = P_tmpdir;
	}

      int tmpdirlen = strlen (tmpdir);
      int suffixlen = strlen (suffix);

      dirnamelen = tmpdirlen + suffixlen + 1;

      dirname = malloc_guarded (dirnamelen + 1);

      sprintf (dirname, "%s/%s", tmpdir, suffix);

      if (mkdtemp (dirname) == NULL)
	{
	  error_quit ("Failed to create main temp directory.");
	}
    }
  else
    {
      if (mkdir (dir, 0700) == -1)
	{
	  if (errno != EEXIST)
	    {
	      error_quit ("Couldn't make tmpdir.");
	    }
	}
      dirname = dir;
      dirnamelen = strlen (dir);
    }

  int i;

  PIXA *templates =
    pixaCreateFromPix (data->pix, data->nclass, data->latticew,
		       data->latticeh);

  if (templates == NULL)
    {
      error_quit ("Could not create templates from JBDATA.");
    }

  /* Create a temp dir for each font */
  char **fontdirnames = malloc_guarded (num_fonts * sizeof (char *));
  int *fontdirlens = malloc_guarded (num_fonts * sizeof (int *));

  for (i = 0; i < num_fonts; i++)
    {
      /* 1 for / 8 for %08d */
      fontdirlens[i] = dirnamelen + 1 + 8;
      fontdirnames[i] = malloc_guarded (fontdirlens[i] + 1);

      sprintf (fontdirnames[i], "%s/%08d", dirname, i);

      if (mkdir (fontdirnames[i], 0700) == -1)
	{
	  error_quit ("Failed to create font temp directory.");
	}
    }


  for (i = 0; i < templates->n; i++)
    {
      l_int32 iclass;
      l_int32 x;
      l_int32 y;

      numaGetIValue (data->naclass, i, &iclass);
      ptaGetIPt (data->ptaul, i, &x, &y);
      PIX *pix_clone = pixaGetPix (templates, i, L_CLONE);	/* the template */
      double wp1 = pixGetWidth (pix_clone);
      double hp1 = pixGetHeight (pix_clone);
      PIX *pix_padded =
	pixAddBorderGeneral (pix_clone, 0, data->latticew - wp1, 0,
			     data->latticeh - hp1, 0);

      if (pix_padded == NULL)
	{
	  error_quit ("Failed to add border to image.");
	}

      int fontn = maps[i].font_num;
      int code_point = maps[i].code_point;

      /* 1 for '/', 3 for %03d, 4 for '.png' */
      int filenamelen = fontdirlens[fontn] + 1 + 3 + 4;
      char *filename = malloc_guarded (filenamelen + 1);

      sprintf (filename, "%s/%03d.png", fontdirnames[fontn], code_point);

      if (pixWrite (filename, pix_padded, IFF_PNG) == 1)
	{
	  printf ("pixWrite failed to write %s.\n", filename);
	  error_quit ("Could not write to file.");
	}

      pixDestroy (&pix_clone);
      pixDestroy (&pix_padded);
      free (filename);
    }

  pixaDestroy (&templates);

  /* This part probably won't port over to Windows as well */

  /* TODO: parallelize this */
  for (i = 0; i < num_fonts; i++)
    {
      /* 1 for '/', 8 for %08d, 4 for '.ttf' */
      int fontnamelen = dirnamelen + 1 + 8 + 4;
      char *fontnamestr = malloc_guarded (fontnamelen + 1);
      sprintf (fontnamestr, "%s/%08d.ttf", dirname, i);

      create_font_from_dir (fontdirnames[i], fontnamestr, data->latticeh,
			    data->latticew, i);
      free (fontnamestr);
    }

  /* clean up */
  for (i = 0; i < num_fonts; i++)
    {
      free (fontdirnames[i]);
    }
  free (fontdirnames);
  free (fontdirlens);

  return dirname;
}

void
generate_pdf (const char *outname, const char *tmpdirname, int num_fonts,
	      int num_input_files, const JBDATA * data,
	      const struct mapping *maps, int debug_draw_borders)
{
  int i, j;
  /* Create the pdf document */
  l_int32 ncomp = numaGetCount (data->naclass);
  HPDF_Doc pdf = HPDF_New (pdf_error_handler, NULL);

  HPDF_SetCompressionMode (pdf, HPDF_COMP_ALL);
  HPDF_Font *fonts = malloc_guarded (num_fonts * (sizeof (HPDF_Font)));

  int dirlen = strlen (tmpdirname);

  /* Load the fonts */
  for (i = 0; i < num_fonts; i++)
    {
      /* 1 for '/', 8 for %08d, 4 for '.ttf' */
      int fontlen = dirlen + 1 + 8 + 4;
      char *font_tfname = malloc_guarded (fontlen + 1);
      sprintf (font_tfname, "%s/%08d.ttf", tmpdirname, i);

      const char *font_name =
	HPDF_LoadTTFontFromFile (pdf, font_tfname, HPDF_TRUE);
      fonts[i] = HPDF_GetFont (pdf, font_name, "KOI8-R");
      free (font_tfname);
    }

  for (j = 0; j < num_input_files; j++)
    {
      /* Add page to document */
      HPDF_Page pg = HPDF_AddPage (pdf);

      HPDF_Page_SetWidth (pg, data->w);
      HPDF_Page_SetHeight (pg, data->h);

      /* TODO: Boost efficiency by making startcomp go to the next page on the next iteration */
      int start_comp = 0;

      for (i = start_comp; i < ncomp; i++)
	{
	  l_int32 ipage;
	  l_int32 iclass;
	  l_int32 x;
	  l_int32 y;

	  numaGetIValue (data->napage, i, &ipage);

	  if (ipage != j)
	    continue;

	  numaGetIValue (data->naclass, i, &iclass);
	  ptaGetIPt (data->ptaul, i, &x, &y);

	  /*double left = x;
	     double top = data->h - y;
	     double right = x + data->latticew;
	     double bottom = data->h - (y + data->latticeh); */

	  char text[2];
	  text[0] = maps[iclass].code_point;
	  text[1] = '\0';

	  HPDF_Font font = fonts[maps[iclass].font_num];

	  HPDF_Page_BeginText (pg);
	  double fontsize = 100;

	  if (fontsize > 300)
	    {
	      error_quit ("This is a known bug.\n"
			  "libharu can't handle fontsizes bigger than 300, which is what is being requested here.\n"
			  "Please report this bug, and the file that produced this error");
	    }

	  HPDF_Page_SetFontAndSize (pg, font, 100);

	  HPDF_Page_MoveTextPos (pg, x, (data->h - y) - data->latticeh);
	  HPDF_Page_ShowText (pg, text);

	  HPDF_Page_EndText (pg);

	  if (debug_draw_borders)
	    {
	      HPDF_Page_SetRGBStroke (pg, 1, 0, 0);
	      /* In this, x, y is the LOWER LEFT, not UPPER LEFT */
	      HPDF_Page_Rectangle (pg, x, (data->h - y) - data->latticeh,
				   data->latticew, data->latticeh);
	      HPDF_Page_Stroke (pg);
	    }
	}
    }

  /* Output */
  HPDF_SaveToFile (pdf, outname);

  /* Cleanup */
  HPDF_Free (pdf);
  free (fonts);
}

JBDATA *
classify_components (int num_input_files, char **input_files, double thresh,
		     double weight)
{

  /* JBCLASSER* classer = jbCorrelationInit(JB_CONN_COMPS, 9999, 9999, thresh, weight); */
  JBCLASSER *classer =
    jbCorrelationInitWithoutComponents (JB_CONN_COMPS, 9999, 9999, thresh,
					weight);

  if (classer == NULL)
    {
      error_quit ("Unable to create leptonica JBCLASSER.");
    }

  int i;

  for (i = 0; i < num_input_files; i++)
    {
      PIX *page = pixRead (input_files[i]);

      if (page == NULL)
	{
	  printf ("Problem with page %s\n", input_files[i]);
	  error_quit ("Unable to read Page");
	}

      if (pixGetDepth (page) != 1)
	{
	  printf ("Input file %s is not 1bpp\n", input_files[i]);
	  error_quit
	    ("Only 1bpp (black and white) images currently supported.");
	}

      if (jbAddPage (classer, page) == 1)
	{
	  printf ("Problem with page %s\n", input_files[i]);
	  error_quit ("Unable to add page to JBCLASSIFIER.");
	}

      pixDestroy (&page);
    }

  /* This is the part we have to add stuff to save each glyph separately */
  JBDATA *data = jbDataSave (classer);

  if (data == NULL)
    {
      error_quit ("Unable to create the leptonica JBDATA.");
    }

  jbClasserDestroy (&classer);

  return data;
}

int
register_mappings (const JBDATA * data, struct mapping **in_maps)
{
  int i;
  /* Register mappings for each component */
  l_int32 ncomp = numaGetCount (data->naclass);

  *in_maps = malloc_guarded (ncomp * sizeof (struct mapping));
  struct mapping *maps = *in_maps;
  for (i = 0; i < ncomp; i++)
    {
      maps[i].used = 0;
    }

  unsigned char code_point = first_code_point ();
  int font_num = 0;

  for (i = 0; i < data->nclass; i++)
    {
      l_int32 iclass = i;

      if (maps[iclass].used)
	continue;

      maps[iclass].iclass = iclass;
      maps[iclass].font_num = font_num;
      maps[iclass].code_point = code_point;
      maps[iclass].used = 1;

      if (code_point == max_code_point ())
	{
	  code_point = first_code_point ();
	  font_num++;
	}
      else
	{
	  code_point = next_code_point (code_point);
	}
    }
  printf ("%d fonts\n", font_num + 1);

  return font_num + 1;
}

struct args *
parse_args (int argc, char *argv[])
{

  if (argc < 2)
    {
      print_help ();
      exit (0);
    }

  struct args *args = malloc_guarded (sizeof (struct args));

  args->num_input_files = 0;
  args->input_files = NULL;
  args->outname = NULL;
  args->thresh = .85;
  args->weight = .5;

  args->help_flag = 0;
  args->version_flag = 0;

  args->debug_tmpdir = NULL;
  args->debug_draw_borders = 0;
  args->debug_render_pages = 0;
  args->debug_skip_font_gen = 0;
  args->debug_no_clean_tmpdir = 0;

  /* Process Command Line args */
  int c;

  struct option long_options[] = {
    {"output", required_argument, 0, 'o'},
    {"help", no_argument, &args->help_flag, 1},
    {"version", no_argument, &args->version_flag, 1},
    {"thresh", required_argument, 0, 't'},
    {"weight", required_argument, 0, 'w'},

    /* Debug options */
    {"debug-tmpdir", required_argument, 0, 0},
    {"debug-draw-borders", no_argument, &args->debug_draw_borders, 1},
    {"debug-render-pages", no_argument, &args->debug_render_pages, 1},
    {"debug-skip-font-gen", no_argument, &args->debug_skip_font_gen, 1},
    {"debug-no-clean-tmpdir", no_argument, &args->debug_no_clean_tmpdir, 1},
    {0, 0, 0, 0}
  };

  while (1)
    {
      int option_index = 0;

      c = getopt_long (argc, argv, "hvo:t:w:", long_options, &option_index);

      if (c == -1)
	break;

      switch (c)
	{
	case 0:
	  {
	    if (strcmp ("debug-tmpdir", long_options[option_index].name) == 0)
	      {
		args->debug_tmpdir = optarg;
	      }
	    break;
	  }
	case 'o':
	  {
	    printf ("output: %s\n", optarg);
	    args->outname = optarg;
	    break;
	  }
	case 't':
	  {
	    double value;
	    sscanf (optarg, "%lf", &value);
	    args->thresh = value;
	    break;
	  }
	case 'w':
	  {
	    double value;
	    sscanf (optarg, "%lf", &value);
	    args->weight = value;
	    break;
	  }
        case 'h':
          {
            args->help_flag = 1;
            break;
          }
        case 'v':
          {
            args->version_flag = 1;
            break;
          }
	default:
	  print_help ();
          free (args);
          exit (EXIT_SUCCESS);
	}
    }

  if (args->help_flag)
    {
      print_help ();
      free (args);
      exit (EXIT_SUCCESS);
    }

  if (args->version_flag)
    {
      print_version ();
      free (args);
      exit (EXIT_SUCCESS);
    }


  if (optind < argc)
    {
      args->num_input_files = argc - optind;
      args->input_files =
	malloc_guarded ((args->num_input_files) * sizeof (char *));
      printf ("%d Input Files:\n", args->num_input_files);
      int k = 0;
      while (optind < argc)
	{
	  args->input_files[k] = argv[optind];
	  optind++;
	  k++;
	}
    }
  else
    {
      error_quit ("No input files specified.");
    }

  return args;
}

int
validate_args (const struct args *args)
{
  int i;
  if (args->num_input_files <= 0 || args->input_files == NULL)
    {
      error_quit ("No input files specified.");
    }
  if (args->outname == NULL)
    {
      error_quit ("No output file specified.");
    }
  /* Check that all input files exist */
  for (i = 0; i < args->num_input_files; i++)
    {
      if (!file_exists (args->input_files[i]))
	{
	  printf ("Can't read %s.\n", args->input_files[i]);
	  error_quit ("Input file doesn't exist.");
	}
    }
  /* 
     Check thresh and weight in valid range
     thresh (value for correlation score: in [0.4 - 0.98])
     weightfactor (corrects thresh for thick characters [0.0 - 1.0])
   */
  if (args->thresh < 0.4 || args->thresh > 0.98)
    {
      error_quit ("Threshold parameter must be in the range [0.4 - 0.98]");
    }
  if (args->weight < 0.0 || args->weight > 1.0)
    {
      error_quit ("Weight must be in range [0.0 - 1.0]");
    }
  /* Confirm overwriting if outname exists */
  if (file_exists (args->outname))
    {
      char c = 'n';
      printf ("Output file %s already exists. Overwrite? (y/N) ",
	      args->outname);
      scanf ("%c", &c);
      if (c != 'Y' && c != 'y')
	{
	  error_quit ("Output file exists.");
	}
    }

  return 0;
}
