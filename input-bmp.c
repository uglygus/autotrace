/* input-bmp.c	reads any bitmap I could get for testing */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitmap.h"
#include "message.h"
#include "xmem.h"
#include "input-bmp.h"


#define MAXCOLORS       256
#define Image		long

#define BitSet(byte, bit)  (((byte) & (bit)) == (bit))

#define ReadOK(file,buffer,len)  (fread(buffer, len, 1, file) != 0)
#define Write(file,buffer,len)   fwrite(buffer, len, 1, file)
#define WriteOK(file,buffer,len) (Write(buffer, len, file) != 0)

struct Bitmap_File_Head_Struct
{
  char            zzMagic[2];  /* 00 "BM" */
  unsigned long   bfSize;      /* 02 */
  unsigned short  zzHotX;      /* 06 */
  unsigned short  zzHotY;      /* 08 */
  unsigned long   bfOffs;      /* 0A */
  unsigned long   biSize;      /* 0E */
} Bitmap_File_Head;

struct Bitmap_Head_Struct
{
  unsigned long   biWidth;     /* 12 */
  unsigned long   biHeight;    /* 16 */
  unsigned short  biPlanes;    /* 1A */
  unsigned short  biBitCnt;    /* 1C */
  unsigned long   biCompr;     /* 1E */
  unsigned long   biSizeIm;    /* 22 */
  unsigned long   biXPels;     /* 26 */
  unsigned long   biYPels;     /* 2A */
  unsigned long   biClrUsed;   /* 2E */
  unsigned long   biClrImp;    /* 32 */
                        /* 36 */
} Bitmap_Head;

static long        ToL           (unsigned char *);
static short       ToS           (unsigned char *);
static int         ReadColorMap  (FILE *,
				   unsigned char[256][3],
				   int,
				   int,
				   int *);
static unsigned char        *ReadImage     (FILE *,
				   int,
				   int,
				   unsigned char[256][3],
				   int,
				   int,
				   int,
				   int);
static void         WriteColorMap (FILE *,
				   int *,
				   int *,
				   int *,
				   int);
static void         WriteImage    (FILE *,
				   unsigned char *,
				   int,
				   int,
				   int,
				   int,
				   int,
				   int,
				   int);

bitmap_type
ReadBMP (string filename)
{
  FILE *fd;
  unsigned char buffer[64];
  int ColormapSize, rowbytes, Maps, Grey;
  unsigned char ColorMap[256][3];
  bitmap_type image;

  fd = fopen (filename, "rb");

  if (!fd)
      FATAL1 ("Can't open \"%s\"\n", filename);

  /* It is a File. Now is it a Bitmap? Read the shortest possible header.*/

  if (!ReadOK(fd, buffer, 18) || (strncmp((const char *)buffer,"BM",2)))
      FATAL ("Not a valid BMP file %s\n");

  /* bring them to the right byteorder. Not too nice, but it should work */

  Bitmap_File_Head.bfSize    = ToL (&buffer[0x02]);
  Bitmap_File_Head.zzHotX    = ToS (&buffer[0x06]);
  Bitmap_File_Head.zzHotY    = ToS (&buffer[0x08]);
  Bitmap_File_Head.bfOffs    = ToL (&buffer[0x0a]);
  Bitmap_File_Head.biSize    = ToL (&buffer[0x0e]);

  /* What kind of bitmap is it? */

  if (Bitmap_File_Head.biSize == 12) /* OS/2 1.x ? */
    {
      if (!ReadOK (fd, buffer, 8))
          FATAL ("Error reading BMP file header\n");

      Bitmap_Head.biWidth    = ToS (&buffer[0x00]);   /* 12 */
      Bitmap_Head.biHeight   = ToS (&buffer[0x02]);   /* 14 */
      Bitmap_Head.biPlanes   = ToS (&buffer[0x04]);   /* 16 */
      Bitmap_Head.biBitCnt   = ToS (&buffer[0x06]);   /* 18 */
	  Bitmap_Head.biCompr = 0;
	  Bitmap_Head.biSizeIm = 0;
	  Bitmap_Head.biXPels = Bitmap_Head.biYPels = 0;
	  Bitmap_Head.biClrUsed = 0;
      Maps = 3;
    }
   else if (Bitmap_File_Head.biSize == 40) /* Windows 3.x */
    {
      if (!ReadOK (fd, buffer, Bitmap_File_Head.biSize - 4))
          FATAL ("Error reading BMP file header\n");

      Bitmap_Head.biWidth   =ToL (&buffer[0x00]);       /* 12 */
      Bitmap_Head.biHeight  =ToL (&buffer[0x04]);       /* 16 */
      Bitmap_Head.biPlanes  =ToS (&buffer[0x08]);       /* 1A */
      Bitmap_Head.biBitCnt  =ToS (&buffer[0x0A]);       /* 1C */
      Bitmap_Head.biCompr   =ToL (&buffer[0x0C]);       /* 1E */
      Bitmap_Head.biSizeIm  =ToL (&buffer[0x10]);       /* 22 */
      Bitmap_Head.biXPels   =ToL (&buffer[0x14]);       /* 26 */
      Bitmap_Head.biYPels   =ToL (&buffer[0x18]);       /* 2A */
      Bitmap_Head.biClrUsed =ToL (&buffer[0x1C]);       /* 2E */
      Bitmap_Head.biClrImp  =ToL (&buffer[0x20]);       /* 32 */
                                                        /* 36 */
      Maps = 4;
    }
  else if (Bitmap_File_Head.biSize <= 64) /* Probably OS/2 2.x */
    {
      if (!ReadOK (fd, buffer, Bitmap_File_Head.biSize - 4))
          FATAL ("Error reading BMP file header\n");
        
      Bitmap_Head.biWidth   =ToL (&buffer[0x00]);       /* 12 */
      Bitmap_Head.biHeight  =ToL (&buffer[0x04]);       /* 16 */
      Bitmap_Head.biPlanes  =ToS (&buffer[0x08]);       /* 1A */
      Bitmap_Head.biBitCnt  =ToS (&buffer[0x0A]);       /* 1C */
      Bitmap_Head.biCompr   =ToL (&buffer[0x0C]);       /* 1E */
      Bitmap_Head.biSizeIm  =ToL (&buffer[0x10]);       /* 22 */
      Bitmap_Head.biXPels   =ToL (&buffer[0x14]);       /* 26 */
      Bitmap_Head.biYPels   =ToL (&buffer[0x18]);       /* 2A */
      Bitmap_Head.biClrUsed =ToL (&buffer[0x1C]);       /* 2E */
      Bitmap_Head.biClrImp  =ToL (&buffer[0x20]);       /* 32 */
                                                        /* 36 */
      Maps = 3;
    }
  else
      FATAL ("Error reading BMP file header\n");

  /* Valid options 1, 4, 8, 16, 24, 32 */
  /* 16 is awful, we should probably shoot whoever invented it */
  
  /* There should be some colors used! */
  
  ColormapSize = (Bitmap_File_Head.bfOffs - Bitmap_File_Head.biSize - 14) / Maps;

  if ((Bitmap_Head.biClrUsed == 0) && (Bitmap_Head.biBitCnt <= 8))
    Bitmap_Head.biClrUsed = ColormapSize;

  /* Sanity checks */

  if (Bitmap_Head.biHeight == 0 || Bitmap_Head.biWidth == 0)
      FATAL ("Error reading BMP file header");

  if (Bitmap_Head.biPlanes != 1)
      FATAL ("Error reading BMP file header");

  if (ColormapSize > 256 || Bitmap_Head.biClrUsed > 256)
      FATAL ("Error reading BMP file header");

  /* Windows and OS/2 declare filler so that rows are a multiple of
   * word length (32 bits == 4 bytes)
   */

  rowbytes= ( (Bitmap_Head.biWidth * Bitmap_Head.biBitCnt - 1) / 32) * 4 + 4;  

#ifdef DEBUG
  printf("\nSize: %u, Colors: %u, Bits: %u, Width: %u, Height: %u, Comp: %u, Zeile: %u\n",
          Bitmap_File_Head.bfSize,Bitmap_Head.biClrUsed,Bitmap_Head.biBitCnt,Bitmap_Head.biWidth,
          Bitmap_Head.biHeight, Bitmap_Head.biCompr, rowbytes);
#endif

  /* Get the Colormap */

  if (ReadColorMap (fd, ColorMap, ColormapSize, Maps, &Grey) == -1)
      FATAL ("Cannot read the colormap");

#ifdef DEBUG
  printf("Colormap read\n");
#endif

  /* Get the Image and return the ID or -1 on error*/
  image.bitmap = ReadImage (fd,
			Bitmap_Head.biWidth,
			Bitmap_Head.biHeight,
			ColorMap,
			Bitmap_Head.biBitCnt,
			Bitmap_Head.biCompr,
			rowbytes,
			Grey);
  BITMAP_WIDTH (image) = Bitmap_Head.biWidth;
  BITMAP_HEIGHT (image) = Bitmap_Head.biHeight;
  BITMAP_PLANES (image) = Grey ? 1 : 3;

  return (image);
}

static int
ReadColorMap (FILE   *fd,
	      unsigned char  buffer[256][3],
	      int    number,
	      int    size,
	      int   *grey)
{
  int i;
  unsigned char rgb[4];

  *grey=(number>2);
  for (i = 0; i < number ; i++)
    {
      if (!ReadOK (fd, rgb, size))
          FATAL ("Bad colormap\n");

      /* Bitmap save the colors in another order! But change only once! */

      buffer[i][0] = rgb[2];
      buffer[i][1] = rgb[1];
      buffer[i][2] = rgb[0];
      *grey = ((*grey) && (rgb[0]==rgb[1]) && (rgb[1]==rgb[2]));
    }
  return 0;
}

static unsigned char*
ReadImage (FILE   *fd,
	   int    width,
	   int    height,
	   unsigned char  cmap[256][3],
	   int    bpp,
	   int    compression,
	   int    rowbytes,
	   int    grey)
{
  unsigned char v,howmuch;
  int xpos = 0, ypos = 0;
  unsigned char *image;
  unsigned char *temp, *buffer;
  long rowstride, channels;
  unsigned short rgb;
  int i, j, notused;

  if (bpp >= 16) /* color image */
    {
      XMALLOC (image, width * height * 3 * sizeof (unsigned char));
      channels = 3;
    }
  else if (grey) /* grey image */
    {
      XMALLOC (image, width * height * 1 * sizeof (unsigned char));
	  channels = 1;
	}
  else /* indexed image */
	{
      XMALLOC (image, width * height * 1 * sizeof (unsigned char));
	  channels = 1;
	}

  XMALLOC (buffer, rowbytes); 
  rowstride = width * channels;

  ypos = height - 1;  /* Bitmaps begin in the lower left corner */

  switch (bpp) {

  case 32:
    {
      while (ReadOK (fd, buffer, rowbytes))
        {
          temp = image + (ypos * rowstride);
          for (xpos= 0; xpos < width; ++xpos)
            {
               *(temp++)= buffer[xpos * 4 + 2];
               *(temp++)= buffer[xpos * 4 + 1];
               *(temp++)= buffer[xpos * 4];
            }
          --ypos; /* next line */
        }
    }
	break;

  case 24:
    {
      while (ReadOK (fd, buffer, rowbytes))
        {
          temp = image + (ypos * rowstride);
          for (xpos= 0; xpos < width; ++xpos)
            {
               *(temp++)= buffer[xpos * 3 + 2];
               *(temp++)= buffer[xpos * 3 + 1];
               *(temp++)= buffer[xpos * 3];
            }
          --ypos; /* next line */
        }
	}
    break;

  case 16:
    {
      while (ReadOK (fd, buffer, rowbytes))
        {
          temp = image + (ypos * rowstride);
          for (xpos= 0; xpos < width; ++xpos)
            {
               rgb= ToS(&buffer[xpos * 2]);
               *(temp++)= (unsigned char)(((rgb >> 10) & 0x1f) * 8);
               *(temp++)= (unsigned char)(((rgb >> 5)  & 0x1f) * 8);
               *(temp++)= (unsigned char)(((rgb)       & 0x1f) * 8);
            }
          --ypos; /* next line */
        }
    }
	break;

  case 8:
  case 4:
  case 1:
    {
      if (compression == 0)
	  {
	    while (ReadOK (fd, &v, 1))
	      {
		for (i = 1; (i <= (8 / bpp)) && (xpos < width); i++, xpos++)
		  {
		    temp = image + (ypos * rowstride) + (xpos * channels);
		    *temp=( v & ( ((1<<bpp)-1) << (8-(i*bpp)) ) ) >> (8-(i*bpp));
		    if (grey)
		      *temp = cmap[*temp][0];
		  }
		if (xpos == width)
		  {
		    notused = ReadOK (fd, buffer, rowbytes - 1 -
                                                (width * bpp - 1) / 8);
		    ypos--;
		    xpos = 0;

		  }
		if (ypos < 0)
		  break;
	      }
	    break;
	  }
	else
	  {
	    while (ypos >= 0 && xpos <= width)
	      {
		notused = ReadOK (fd, buffer, 2);
		if ((unsigned char) buffer[0] != 0) 
		  /* Count + Color - record */
		  {
		    for (j = 0; ((unsigned char) j < (unsigned char) buffer[0]) && (xpos < width);)
		      {
#ifdef DEBUG2
			printf("%u %u | ",xpos,width);
#endif
			for (i = 1;
			     ((i <= (8 / bpp)) &&
			      (xpos < width) &&
			      ((unsigned char) j < (unsigned char) buffer[0]));
			     i++, xpos++, j++)
			  {
			    temp = image + (ypos * rowstride) + (xpos * channels);
			    *temp = (buffer[1] & (((1<<bpp)-1) << (8 - (i * bpp)))) >> (8 - (i * bpp));
			    if (grey)
			      *temp = cmap[*temp][0];
			  }
		      }
		  }
		if (((unsigned char) buffer[0] == 0) && ((unsigned char) buffer[1] > 2))
		  /* uncompressed record */
		  {
		    howmuch = buffer[1];
		    for (j = 0; j < howmuch; j += (8 / bpp))
		      {
			notused = ReadOK (fd, &v, 1);
			i = 1;
			while ((i <= (8 / bpp)) && (xpos < width))
			  {
			    temp = image + (ypos * rowstride) + (xpos * channels);
			    *temp = (v & (((1<<bpp)-1) << (8-(i*bpp)))) >> (8-(i*bpp));
			    if (grey)
			      *temp = cmap[*temp][0];
			    i++;
			    xpos++;
			  }
		      }

		    if ((howmuch % 2) && (bpp==4))
		      howmuch++;

		    if ((howmuch / (8 / bpp)) % 2)
		      notused = ReadOK (fd, &v, 1);
		    /*if odd(x div (8 div bpp )) then blockread(f,z^,1);*/
		  }
		if (((unsigned char) buffer[0] == 0) && ((unsigned char) buffer[1]==0))
		  /* Line end */
		  {
		    ypos--;
		    xpos = 0;
		  }
		if (((unsigned char) buffer[0]==0) && ((unsigned char) buffer[1]==1))
		  /* Bitmap end */
		  {
		    break;
		  }
		if (((unsigned char) buffer[0]==0) && ((unsigned char) buffer[1]==2))
		  /* Deltarecord */
		  {
		    notused = ReadOK (fd, buffer, 2);
		    xpos += (unsigned char) buffer[0];
		    ypos -= (unsigned char) buffer[1];
		  }
	      }
	    break;
	  }
    }
    break;
  default:
    /* This is very bad, we should not be here */
	;
  }

  fclose (fd);
  if (bpp <= 8)
    {
      unsigned char *temp2, *temp3;
      unsigned char index;
      temp2 = temp = image;
      XMALLOC (image, width * height * 3 * sizeof (unsigned char));
      temp3 = image;
      for (ypos = 0; ypos < height; ypos++)
        {
          for (xpos = 0; xpos < width; xpos++)
             {
               index = *temp2++;
               *temp3++ = cmap[index][0];
			   if (!grey)
			     {
                   *temp3++ = cmap[index][1];
                   *temp3++ = cmap[index][2];
			     }
           }
        }
      free (temp);
  }

  free (buffer);
  return image;
}

FILE  *errorfile;
char *prog_name = "bmp";
char *filename;
int   interactive_bmp;

static long
ToL (unsigned char *puffer)
{
  return (puffer[0] | puffer[1]<<8 | puffer[2]<<16 | puffer[3]<<24);
}

static short
ToS (unsigned char *puffer)
{
  return ((short)(puffer[0] | puffer[1]<<8));
}

/* version 0.26 */
