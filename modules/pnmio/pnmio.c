/*
 * File       : pnmio.c                                                          
 * Description: I/O facilities for PBM, PGM, PPM (PNM) binary and ASCII images.
 * Author     : Nikolaos Kavvadias <nikolaos.kavvadias@gmail.com>                
 * Copyright  : (C) Nikolaos Kavvadias 2012, 2013, 2014, 2015, 2016, 2017
 * Website    : http://www.nkavvadias.com                            
 *                                                                          
 * This file is part of libpnmio, and is distributed under the terms of the  
 * Modified BSD License.
 *
 * A copy of the Modified BSD License is included with this distribution 
 * in the file LICENSE.
 * libpnmio is free software: you can redistribute it and/or modify it under the
 * terms of the Modified BSD License. 
 * libpnmio is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the Modified BSD License for more details.
 * 
 * You should have received a copy of the Modified BSD License along with 
 * libpnmio. If not, see <http://www.gnu.org/licenses/>. 
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include "pnmio.h"

#define  MAXLINE         128 //1024
#define  LITTLE_ENDIAN     -1
#define  BIG_ENDIAN         1
#define  GREYSCALE_TYPE     0 /* used for PFM */
#define  RGB_TYPE           1 /* used for PFM */   



//#define exit(a) return void
//#define fprintf(a, b, c, d) printf(b, c, d)
#define printf(...) 

/* get_pnm_type:
 * Read the header contents of a PBM/PGM/PPM/PFM file up to the point of 
 * extracting its type. Valid types for a PNM image are as follows:
 *   PBM_ASCII     =  1
 *   PGM_ASCII     =  2
 *   PPM_ASCII     =  3
 *   PBM_BINARY    =  4
 *   PGM_BINARY    =  5
 *   PPM_BINARY    =  6
 *   PAM           =  7 
 *   PFM_RGB       = 16 
 *   PFM_GREYSCALE = 17
 * 
 * The result (pnm_type) is returned.
 */
int get_pnm_type(FILE *f)
{
  int flag=0;
  int pnm_type=0;
  unsigned int i;
//  char magic[MAXLINE];
//  char line[MAXLINE];
  char *magic;
  char *line;
  
  line = malloc(MAXLINE);
  magic = line;
 
  /* Read the PNM/PFM file header. */
  while (fgets(line, MAXLINE, f) != NULL) {
    flag = 0;
    for (i = 0; i < strlen(line); i++) {
      if (isgraph(line[i])) {
        if ((line[i] == '#') && (flag == 0)) {
          flag = 1;
        }
      }
    }
    if (flag == 0) {
      sscanf(line, "%s", magic);
      break;
    }
  }

  /* NOTE: This part can be written more succinctly, however, 
   * it is better to have the PNM types decoded explicitly.
   */
  if (strcmp(magic, "P1") == 0) {
    pnm_type = PBM_ASCII;
/*
  } else if (strcmp(magic, "P2") == 0) {
    pnm_type = PGM_ASCII;
  } else if (strcmp(magic, "P3") == 0) {
    pnm_type = PPM_ASCII;
*/
  } else if (strcmp(magic, "P4") == 0) {
    pnm_type = PBM_BINARY;
/*
  } else if (strcmp(magic, "P5") == 0) {
    pnm_type = PGM_BINARY;
  } else if (strcmp(magic, "P6") == 0) {
    pnm_type = PPM_BINARY;
  } else if (strcmp(magic, "P7") == 0) {
    pnm_type = PAM;
  } else if (strcmp(magic, "PF") == 0) {
    pnm_type = PFM_RGB;
  } else if (strcmp(magic, "Pf") == 0) {
    pnm_type = PFM_GREYSCALE;
*/
  } else {
    printf("Error: Unknown PNM/PFM file; wrong magic number!\n");
//    exit(1);
	pnm_type = 0;
  }

  free(line);
  
  return (pnm_type);
}

/* read_pbm_header:
 * Read the header contents of a PBM (Portable Binary Map) file.
 * An ASCII PBM image file follows the format:
 * P1
 * <X> <Y>
 * <I1> <I2> ... <IMAX>
 * A binary PBM image file uses P4 instead of P1 and 
 * the data values are represented in binary. 
 * NOTE1: Comment lines start with '#'.
 * NOTE2: < > denote integer values (in decimal).
 */
void read_pbm_header(FILE *f, int *img_xdim, int *img_ydim, int *is_ascii)
{
  int flag=0;
  int x_val, y_val;
  unsigned int i;
//  char magic[MAXLINE];
//  char line[MAXLINE];
  char magic = '9';
  char *line;
  int count=0;

  *is_ascii = 9;
  x_val = y_val = 0;
  
  line = malloc(MAXLINE);
//  magic = malloc(MAXLINE);
  
  /* Read the PBM file header. */
  while (fgets(line, MAXLINE, f) != NULL) {
    flag = 0;
    for (i = 0; i < strlen(line); i++) {
      if (isgraph(line[i])) {
        if ((line[i] == '#') && (flag == 0)) {
          flag = 1;
        }
      }
    }
    if (flag == 0) {
      if (count == 0) {
        count += sscanf(line, "P%c", &magic); //, &x_val, &y_val);
      } else if (count == 1) {
        count += sscanf(line, "%d %d", &x_val, &y_val);
      } else if (count == 2) {
        count += sscanf(line, "%d", &y_val);
      }
    }
    if (count > 2) {
      break;
    }
  }

//  if (strcmp(magic, "P1") == 0) {
  if (magic == '1') {
    *is_ascii = 1;
//  } else if (strcmp(magic, "P4") == 0) {
  } else if (magic == '4') {
    *is_ascii = 0;
  } else {
    printf("Error: Input file not in PBM format!\n");
    //exit(1);
//    free(line);
//	free(magic);
//    return;
  }

  printf("Info: magic=%s, x_val=%d, y_val=%d\n", magic, x_val, y_val);
  *img_xdim   = x_val;
  *img_ydim   = y_val;
  
  free(line);
//  free(magic);
}

#if 0
/* read_pgm_header:
 * Read the header contents of a PGM (Portable Grey[scale] Map) file.
 * An ASCII PGM image file follows the format:
 * P2
 * <X> <Y> 
 * <levels>
 * <I1> <I2> ... <IMAX>
 * A binary PGM image file uses P5 instead of P2 and 
 * the data values are represented in binary. 
 * NOTE1: Comment lines start with '#'.
 * NOTE2: < > denote integer values (in decimal).
 */
void read_pgm_header(FILE *f, int *img_xdim, int *img_ydim, int *img_colors, int *is_ascii)
{
  int flag=0;
  int x_val, y_val, maxcolors_val;
  unsigned int i;
  char magic[MAXLINE];
  char line[MAXLINE];
  int count=0;

  /* Read the PGM file header. */
  while (fgets(line, MAXLINE, f) != NULL) {   
    flag = 0;
    for (i = 0; i < strlen(line); i++) {
      if (isgraph(line[i]) && (flag == 0)) {
        if ((line[i] == '#') && (flag == 0)) {
          flag = 1;
        }
      }
    }
    if (flag == 0) {
      if (count == 0) {
        count += sscanf(line, "%s %d %d %d", magic, &x_val, &y_val, &maxcolors_val); 
      } else if (count == 1) {
        count += sscanf(line, "%d %d %d", &x_val, &y_val, &maxcolors_val);
      } else if (count == 2) {
        count += sscanf(line, "%d %d", &y_val, &maxcolors_val);
      } else if (count == 3) {
        count += sscanf(line, "%d", &maxcolors_val);
      }
    }
    if (count == 4) {
      break;
    }
  }

  if (strcmp(magic, "P2") == 0) {
    *is_ascii = 1;
  } else if (strcmp(magic, "P5") == 0) {
    *is_ascii = 0;
  } else {
    printf("Error: Input file not in PGM format!\n");
    //exit(1);
    return;
  }

  printf("Info: magic=%s, x_val=%d, y_val=%d, maxcolors_val=%d\n", magic, x_val, y_val, maxcolors_val);
  *img_xdim   = x_val;
  *img_ydim   = y_val;
  *img_colors = maxcolors_val;
}

/* read_ppm_header:
 * Read the header contents of a PPM (Portable Pix[el] Map) file.
 * An ASCII PPM image file follows the format:
 * P3
 * <X> <Y> 
 * <colors>
 * <R1> <G1> <B1> ... <RMAX> <GMAX> <BMAX>
 * A binary PPM image file uses P6 instead of P3 and 
 * the data values are represented in binary. 
 * NOTE1: Comment lines start with '#'.
 # NOTE2: < > denote integer values (in decimal).
 */
void read_ppm_header(FILE *f, int *img_xdim, int *img_ydim, int *img_colors, int *is_ascii)
{
  int flag=0;
  int x_val, y_val, maxcolors_val;
  unsigned int i;
  char magic[MAXLINE];
  char line[MAXLINE];
  int count=0;
 
  /* Read the PPM file header. */
  while (fgets(line, MAXLINE, f) != NULL) {
    flag = 0;
    for (i = 0; i < strlen(line); i++) {
      if (isgraph(line[i]) && (flag == 0)) {
        if ((line[i] == '#') && (flag == 0)) {
          flag = 1;
        }
      }
    }
    if (flag == 0) {
      if (count == 0) {
        count += sscanf(line, "%s %d %d %d", magic, &x_val, &y_val, &maxcolors_val); 
      } else if (count == 1) {
        count += sscanf(line, "%d %d %d", &x_val, &y_val, &maxcolors_val);
      } else if (count == 2) {
        count += sscanf(line, "%d %d", &y_val, &maxcolors_val);
      } else if (count == 3) {
        count += sscanf(line, "%d", &maxcolors_val);
      }
    }
    if (count == 4) {
      break;
    }
  }

  if (strcmp(magic, "P3") == 0) {
    *is_ascii = 1;
  } else if (strcmp(magic, "P6") == 0) {
    *is_ascii = 0;
  } else {
    printf("Error: Input file not in PPM format!\n");
    //exit(1);
    return;
  }

  printf("Info: magic=%s, x_val=%d, y_val=%d, maxcolors_val=%d\n", magic, x_val, y_val, maxcolors_val);
  *img_xdim   = x_val;
  *img_ydim   = y_val;
  *img_colors = maxcolors_val;
}

/* read_pfm_header:
 * Read the header contents of a PFM (portable float map) file.
 * A PFM image file follows the format:
 * [PF|Pf]
 * <X> <Y> 
 * (endianess)
 * {R1}{G1}{B1} ... {RMAX}{GMAX}{BMAX} 
 * NOTE1: Comment lines, if allowed, start with '#'.
 # NOTE2: < > denote integer values (in decimal).
 # NOTE3: ( ) denote floating-point values (in decimal).
 # NOTE4: { } denote floating-point values (coded in binary).
 */
void read_pfm_header(FILE *f, int *img_xdim, int *img_ydim, int *img_type, int *endianess)
{
  int flag=0;
  int x_val, y_val;
  unsigned int i;
  int is_rgb=0, is_greyscale=0;
  float aspect_ratio=0;
  char magic[MAXLINE];
  char line[MAXLINE];
  int count=0;

  /* Read the PFM file header. */
  while (fgets(line, MAXLINE, f) != NULL) {
    flag = 0;
    for (i = 0; i < strlen(line); i++) {
      if (isgraph(line[i]) && (flag == 0)) {
        if ((line[i] == '#') && (flag == 0)) {
          flag = 1;
        }
      }
    }
    if (flag == 0) {
      if (count == 0) {
        count += sscanf(line, "%s %d %d %f", magic, &x_val, &y_val, &aspect_ratio); 
      } else if (count == 1) {
        count += sscanf(line, "%d %d %f", &x_val, &y_val, &aspect_ratio);
      } else if (count == 2) {
        count += sscanf(line, "%d %f", &y_val, &aspect_ratio);
      } else if (count == 3) {
        count += sscanf(line, "%f", &aspect_ratio);
      }
    }
    if (count == 4) {
      break;
    }
  }

  if (strcmp(magic, "PF") == 0) {
    is_rgb       = 1;
    is_greyscale = 0;
  } else if (strcmp(magic, "Pf") == 0) {
    is_greyscale = 0;
    is_rgb       = 1;    
  } else {
    printf("Error: Input file not in PFM format!\n");
    //exit(1);
    return;
  }      

  printf("Info: magic=%s, x_val=%d, y_val=%d, aspect_ratio=%f\n", magic, x_val, y_val, aspect_ratio);

  /* FIXME: Aspect ratio different to 1.0 is not yet supported. */
  if (!floatEqualComparison(aspect_ratio, -1.0, 1E-06) &&
      !floatEqualComparison(aspect_ratio, 1.0, 1E-06)) {
    printf("Error: Aspect ratio different to -1.0 or +1.0 is unsupported!\n");
    //exit(1);
    return;
  }

  *img_xdim   = x_val;
  *img_ydim   = y_val;
  *img_type   = is_rgb & ~is_greyscale;
  if (aspect_ratio > 0.0) {
    *endianess = 1;
  } else {
    *endianess = -1;
  }
}
#endif


/* read_pbm_data:
 * Read the data contents of a PBM (portable bit map) file.
 */
void read_pbm_data(FILE *f, char *img_in, int size, int is_ascii)
{
  int i=0, c;
  int lum_val;
  int k;
  
  /* Read the rest of the PBM file. */
  while (i < size && (c = fgetc(f)) != EOF) {
    if (is_ascii == 1) {
	  ungetc(c, f);
      if (fscanf(f, "%d", &lum_val) != 1) return;
      img_in[i++] = lum_val;
    } else {
      //lum_val = fgetc(f);
	  img_in[i++] = c; //lum_val;
//      /* Decode the image contents byte-by-byte. */
//      for (k = 0; k < 8; k++) {
//        img_in[i++] = (lum_val >> (7-k)) & 0x1;
//      }        
    }
  } 
  fclose(f);
}


#if 0
/* read_pgm_data:
 * Read the data contents of a PGM (portable grey map) file.
 */
void read_pgm_data(FILE *f, int *img_in, int is_ascii)
{
  int i=0, c;
  int lum_val;
  
  /* Read the rest of the PGM file. */
  while ((c = fgetc(f)) != EOF) {
    ungetc(c, f);
    if (is_ascii == 1) {
        if (fscanf(f, "%d", &lum_val) != 1) return;
	  } else {
      lum_val = fgetc(f);
    }        
    img_in[i++] = lum_val;
  } 
  fclose(f);
}

/* read_ppm_data:
 * Read the data contents of a PPM (portable pix map) file.
 */
void read_ppm_data(FILE *f, int *img_in, int is_ascii)
{
  int i=0, c;
  int r_val, g_val, b_val;
    
  /* Read the rest of the PPM file. */
  while ((c = fgetc(f)) != EOF) {
    ungetc(c, f);
    if (is_ascii == 1) {
      if (fscanf(f, "%d %d %d", &r_val, &g_val, &b_val) != 3) return;
    } else {
      r_val = fgetc(f);
      g_val = fgetc(f);
      b_val = fgetc(f);
    }
    img_in[i++] = r_val;
    img_in[i++] = g_val;
    img_in[i++] = b_val;
  }
  fclose(f);
}

/* read_pfm_data:
 * Read the data contents of a PFM (portable float map) file.
 */
void read_pfm_data(FILE *f, float *img_in, int img_type, int endianess)
{
  int i=0, c;
  int swap = (endianess == 1) ? 0 : 1;
  float r_val, g_val, b_val;
    
  /* Read the rest of the PFM file. */
  while ((c = fgetc(f)) != EOF) {
    ungetc(c, f);   
    /* Read a possibly byte-swapped float. */
    if (img_type == RGB_TYPE) {
      ReadFloat(f, &r_val, swap);
      ReadFloat(f, &g_val, swap);
      ReadFloat(f, &b_val, swap); 
      img_in[i++] = r_val;
      img_in[i++] = g_val;
      img_in[i++] = b_val;
    } else if (img_type == GREYSCALE_TYPE) {
      ReadFloat(f, &g_val, swap);
      img_in[i++] = g_val;
    }
  }
  fclose(f);
}

/* write_pbm_file:
 * Write the contents of a PBM (portable bit map) file.
 */
void write_pbm_file(FILE *f, int *img_out, char *img_out_fname, 
  int x_size, int y_size, int x_scale_val, int y_scale_val, int linevals,
  int is_ascii)
{
  int i, j, x_scaled_size, y_scaled_size;
  int k, v, temp, step;
 
  x_scaled_size = x_size * x_scale_val;
  y_scaled_size = y_size * y_scale_val; 
  /* Write the magic number string. */
  if (is_ascii == 1) {
    fprintf(f, "P1\n");
	step = 1;
  } else {
    fprintf(f, "P4\n");
	step = 8;
  }
  /* Write a comment containing the file name. */
  fprintf(f, "# %s\n", img_out_fname);
  /* Write the image dimensions. */
  fprintf(f, "%d %d\n", x_scaled_size, y_scaled_size);
  
  /* Write the image data. */
  for (i = 0; i < y_scaled_size; i++) {
    for (j = 0; j < x_scaled_size; j+=step) {
	    if (is_ascii == 1) {
        fprintf(f, "%d ", img_out[i*x_scaled_size+j]);
	    } else {
	      temp = 0;
		    for (k = 0; k < 8; k++) {
          v = img_out[i*x_scaled_size+j+k];
          temp |= (v << (7-k));
		    }
        fprintf(f, "%c", temp);
      }
      if (((i*x_scaled_size+j) % linevals) == (linevals-1)) {
        fprintf(f, "\n");
      }
    }
  }   
  fclose(f);
}

/* write_pgm_file:
 * Write the contents of a PGM (portable grey map) file.
 */
void write_pgm_file(FILE *f, int *img_out, char *img_out_fname, 
  int x_size, int y_size, int x_scale_val, int y_scale_val, 
  int img_colors, int linevals, int is_ascii)
{
  int i, j, x_scaled_size, y_scaled_size;
 
  x_scaled_size = x_size * x_scale_val;
  y_scaled_size = y_size * y_scale_val; 
  /* Write the magic number string. */
  if (is_ascii == 1) {
    fprintf(f, "P2\n");
  } else {
    fprintf(f, "P5\n");
  }
  /* Write a comment containing the file name. */
  fprintf(f, "# %s\n", img_out_fname);
  /* Write the image dimensions. */
  fprintf(f, "%d %d\n", x_scaled_size, y_scaled_size);
  /* Write the maximum color/grey level allowed. */
  fprintf(f, "%d\n", img_colors);
  
  /* Write the image data. */
  for (i = 0; i < y_scaled_size; i++) {
    for (j = 0; j < x_scaled_size; j++) {
      if (is_ascii == 1) {
        fprintf(f, "%d ", img_out[i*x_scaled_size+j]);
        if (((i*x_scaled_size+j) % linevals) == (linevals-1)) {
          fprintf(f, "\n");
        }
      } else {
        fprintf(f, "%c", img_out[i*x_scaled_size+j]);
      }
    }
  } 
  fclose(f);
}

/* write_ppm_file:
 * Write the contents of a PPM (portable pix map) file.
 */
void write_ppm_file(FILE *f, int *img_out, char *img_out_fname, 
  int x_size, int y_size, int x_scale_val, int y_scale_val, 
  int img_colors, int is_ascii)
{
  int i, j, x_scaled_size, y_scaled_size;
  
  x_scaled_size = x_size * x_scale_val;
  y_scaled_size = y_size * y_scale_val;
  /* Write the magic number string. */
  if (is_ascii == 1) {
    fprintf(f, "P3\n");
  } else {
    fprintf(f, "P6\n");
  }
  /* Write a comment containing the file name. */
  fprintf(f, "# %s\n", img_out_fname);
  /* Write the image dimensions. */
  fprintf(f, "%d %d\n", x_scaled_size, y_scaled_size);
  /* Write the maximum color/grey level allowed. */
  fprintf(f, "%d\n", img_colors);
  
  /* Write the image data. */
  for (i = 0; i < y_scaled_size; i++) {
    for (j = 0; j < x_scaled_size; j++) {
      if (is_ascii == 1) {
        fprintf(f, "%d %d %d ", 
          img_out[3*(i*x_scaled_size+j)+0], 
          img_out[3*(i*x_scaled_size+j)+1], 
          img_out[3*(i*x_scaled_size+j)+2]);
        if ((j % 4) == 0) {
          fprintf(f, "\n");
        }
      } else {
        fprintf(f, "%c%c%c", 
          img_out[3*(i*x_scaled_size+j)+0], 
          img_out[3*(i*x_scaled_size+j)+1], 
          img_out[3*(i*x_scaled_size+j)+2]);
      }
    }
  }  
  fclose(f);
}

/* write_pfm_file:
 * Write the contents of a PFM (portable float map) file.
 */
void write_pfm_file(FILE *f, float *img_out, char *img_out_fname, 
  int x_size, int y_size, 
  int img_type, int endianess)
{
  int i, j, x_scaled_size, y_scaled_size;
  int swap = (endianess == 1) ? 0 : 1;
  float fendian = (endianess == 1) ? +1.0 : -1.0;
  
  x_scaled_size = x_size;
  y_scaled_size = y_size;

  /* Write a comment containing the file name. */
  fprintf(f, "# %s\n", img_out_fname);
  /* Write the magic number string. */
  if (img_type == RGB_TYPE) {
    fprintf(f, "PF\n");
  } else if (img_type == GREYSCALE_TYPE) {
    fprintf(f, "Pf\n");
  } else {
    fprintf(stderr, "Error: Image type invalid for PFM format!\n");
    exit(1);    
  }
  /* Write the image dimensions. */
  fprintf(f, "%d %d\n", x_scaled_size, y_scaled_size);
  /* Write the endianess/scale factor as float. */
  fprintf(f, "%f\n", fendian);
  
  /* Write the image data. */
  for (i = 0; i < y_scaled_size; i++) {
    for (j = 0; j < x_scaled_size; j++) {
      if (img_type == RGB_TYPE) {
        WriteFloat(f, &img_out[3*(i*x_scaled_size+j)+0], swap);
        WriteFloat(f, &img_out[3*(i*x_scaled_size+j)+1], swap);
        WriteFloat(f, &img_out[3*(i*x_scaled_size+j)+2], swap);
      } else if (img_type == GREYSCALE_TYPE) {
        WriteFloat(f, &img_out[i*x_scaled_size+j], swap);
      }
    }
  }  
  fclose(f);
}
#endif

#if 0
/* ReadFloat:
 * Read a possibly byte swapped floating-point number.
 * NOTE: Assume IEEE format.
 * Source: http://paulbourke.net/dataformats/pbmhdr/
 */
int ReadFloat(FILE *fptr, float *f, int swap)
{
  unsigned char *cptr, tmp;

  if (fread(f, sizeof(float), 1, fptr) != 1) {
    return (FALSE);
  }   
  if (swap) {
    cptr    = (unsigned char *)f;
    tmp     = cptr[0];
    cptr[0] = cptr[3];
    cptr[3] = tmp;
    tmp     = cptr[1];
    cptr[1] = cptr[2];
    cptr[2] = tmp;
  }
  return (TRUE);
}

/* WriteFloat:
 * Write a possibly byte-swapped floating-point number.
 * NOTE: Assume IEEE format.
 */
int WriteFloat(FILE *fptr, float *f, int swap)
{
  unsigned char *cptr, tmp;

  if (swap) {
    cptr    = (unsigned char*)f;
    tmp     = cptr[0];
    cptr[0] = cptr[3];
    cptr[3] = tmp;
    tmp     = cptr[1];
    cptr[1] = cptr[2];
    cptr[2] = tmp;
  }
  if (fwrite(f, sizeof(float), 1, fptr) != 1) {
    return (FALSE);
  }  
  return (TRUE); 
}

/* floatEqualComparison: 
 * Compare two floats and accept equality if not different than
 * maxRelDiff (a specified maximum relative difference).
 */
int floatEqualComparison(float A, float B, float maxRelDiff)
{
  float largest, diff = fabs(A-B);
  A = fabs(A);
  B = fabs(B);
  largest = (B > A) ? B : A;
  if (diff <= largest * maxRelDiff) {
    return 1;
  }
  return 0;
}

/* frand48:
 * Emulate a floating-point PRNG.
 * Source: http://c-faq.com/lib/rand48.html
 */
float frand(void)
{
  return rand() / (RAND_MAX + 1.0);
}

/* log2ceil:
 * Function to calculate the ceiling of the binary logarithm of a given positive 
 * integer n.
 */
int log2ceil(int inpval)
{
  int max = 1; // exp=0 => max=2^0=1
  unsigned int logval = 0;

  if (inpval < 0) {
    printf("Error: Result of log2 computation is NAN.\n");
    //exit(1);
    return 0;
  } else if (inpval == 0) {
    printf("Error: Result of log2 computation is MINUS_INFINITY.\n");
    //exit(1);
    return 0;
  } else {
    // log computation loop
    while (max < inpval) {
      // increment exponent
      logval = logval + 1;
      //  max = 2^logval
      max = max * 2;
    }
  }
  // exponent that gives (2^logval) >= inpval
  return (logval);
}
#endif

