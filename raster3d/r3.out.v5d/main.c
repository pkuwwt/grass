/*
 * r3.out.v5d -
 *
 * Copyright Jaro Hofierka
 * GeoModel,s.r.o., Bratislava, 1999
 * hofierka@geomodel.sk
 *
 * Improvements: 
 * - added true coordinates support Markus Neteler 1/2001
 * - Region sensivity by MN 1/2001
 * - Fixed coordinate being reversed MN 1/2001
 *
 * BUGS: see BUG file
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "binio.h"
#include "v5d.h"
#include <grass/gis.h>
#include <grass/G3d.h>
#include <grass/glocale.h>

#define MAX(a,b) (a > b ? a : b)

/* structs */
typedef	struct {
  struct Option *input, *output;
} paramType;

/* protos */
void fatalError(char *errorMsg);
void setParams();
void getParams(char **input, char **output, int *decim);
void convert(char *fileout, int, int, int, int);

/* globals */
void *map = NULL;
paramType param;
G3D_Region region;
   
/*---------------------------------------------------------------------------*/
/* Simple error handling routine, will eventually replace this with
 * G3D_fatalError.
 */
void fatalError(char *errorMsg) {
  if (map != NULL) {
    /* should unopen map here! */
    if (!G3d_closeCell (map))
       fatalError ("Error closing 3d raster map");
  }
  
  G3d_fatalError (errorMsg);
}

/*---------------------------------------------------------------------------*/
/* Convenient way to set up the arguments we are expecting
 */
void setParams() {
  param.input = G_define_option();
  param.input->key = "input";
  param.input->type = TYPE_STRING;
  param.input->required = YES;
  param.input->gisprompt = "old,grid3,3d-raster";
  param.input->multiple = NO;
  param.input->description = _("3d raster map to be converted to Vis5d (v5d) file");
  
  param.output = G_define_option();
  param.output->key = "output";
  param.output->type = TYPE_STRING;
  param.output->gisprompt = "new_file,file,output";
  param.output->required = YES;
  param.output->description = _("Name for v5d output file");

/*  param.null_val = G_define_option();
  param.null_val->key = "null";
  param.null_val->type = TYPE_STRING;
  param.null_val->required = NO;
  param.null_val->description = "Char string to represent no data cell";
  param.null_val->answer = "*";
*/
}

/*---------------------------------------------------------------------------*/
/* Set up the input and output file names from the user's responses
 */
void getParams(char **input, char **output, int *decim) {
  *input = param.input->answer;
  *output = param.output->answer;
}

/*---------------------------------------------------------------------------*/
/* Opens the output v5d file and writes the header.
 * Returns the file handle for the output file.
 */

void convert(char *fileout, int rows, int cols, int depths, int trueCoords) {

   int NumTimes=1;                        /* number of time steps */
   int NumVars=1;                         /* number of variables */
   int Nl[MAXVARS];                      /* size of 3-D grids */
   char VarName[MAXVARS][10];           /* names of variables */
   int TimeStamp[MAXTIMES];             /* real times for each time step */
   int DateStamp[MAXTIMES];             /* real dates for each time step */
   float NorthLat;                      /* latitude of north bound of box */
   float LatInc;                        /* spacing between rows in degrees */
   float WestLon;                       /* longitude of west bound of box */
   float LonInc;                        /* spacing between columns in degs */
   float BottomHgt;                     /* height of bottom of box in km */
   float HgtInc;                        /* spacing between grid levels in km */
   int Projection;
   float ProjArgs[100];
   int Vertical;
   float VertArgs[MAXLEVELS];
   int CompressMode;
   float *g;
   int cnt;
   double d1 = 0;
   double *d1p;
   float *f1p;
   int x, y, z;
   int typeIntern;


/*AV*/
/* BEGIN OF ORIGINAL CODE WHICH IS NOT NECESSARY FOR ME, COMMENTED IT */
/* values of global variables are passed as function's arguments at line 345 */
/*
   / * copy setting from global variables MN 1/2001 * /
   rows = region.rows;
   cols=region.cols;
   depths=region.depths;
*/
/* END OF ORIGINAL CODE WHICH IS NOT NECESSARY FOR ME, COMMENTED IT */


   typeIntern = G3d_tileTypeMap (map);

   G_debug(3, "cols: %i rows: %i depths: %i\n", cols, rows, depths);

  /* see v5d.h */
  if (cols > MAXCOLUMNS)
  	 G_fatal_error(_("Viz5D allows %d cols, you have %d cols"), MAXCOLUMNS, cols);
  if (rows > MAXROWS)
	 G_fatal_error(_("Viz5D allows %d rows, you have %d rows"), MAXROWS, rows);

   Nl[0]=depths;

/* ********* */
/* BUG: vis5d display one row/col/depth less that volume
 *
 * Note: The coordinate system of Vis5D is really odd!
 */

   strcpy(VarName[0], "S");
   TimeStamp[0] = DateStamp[0] = 0;
   CompressMode = 4;

  if(trueCoords)
  { /* use map coordinates */
   Projection = 0;               /*linear, rectangular, generic units*/
   ProjArgs[0] = region.north;   /*North boundary of 3-D box*/
   ProjArgs[1] = region.west;    /*West boundary of 3-D box */
   ProjArgs[2] = region.ns_res;  /*Increment between rows */
   ProjArgs[3] = region.ew_res * (-1);   /*Increment between columns, reverse direction*/
   Vertical    = 0;               /*equally spaced levels in generic units*/
   VertArgs[0] = region.bottom;   /*height of bottom level*/
   VertArgs[1] = region.tb_res;   /*spacing between levels*/
  }
  else
  { /* xyz coordinates */
    Projection = 0;      /*linear, rectangular, generic units*/
    ProjArgs[0] = 0.0;   /*North boundary of 3-D box*/
    ProjArgs[1] = 0.0;   /*West boundary of 3-D box */
    ProjArgs[2] = 1.0;   /*Increment between rows */
    ProjArgs[3] = 1.0;   /*Increment between columns*/
    Vertical    = 0;     /*equally spaced levels in generic units*/
    VertArgs[0] = 0.0;   /*height of bottom level*/
    VertArgs[1] = 1.0;   /*spacing between levels*/
  }


/* put here some g3d functions */
  /* required ? */
        LatInc = 1.0;
        LonInc = 1.0;
        HgtInc = 1.0;
        NorthLat = 50.0;
        WestLon = 90.0;
        BottomHgt = 0.0;
/****************/

   g = (float *) G_malloc ( rows * cols * Nl[0] * sizeof(float) );
   if (!g)
      G_fatal_error(_("Out of memory"));

   d1p = &d1; f1p = (float *) &d1;
   cnt=0;

   /* originally written in 1999. Bug: displays reversed in Vis5D:
      for (z = 0; z < depths; z++) {
      G_percent(z, depths, 1);
      for (y = 0; y < rows; y++) {
      for (x = 0; x < cols; x++) {
   */

   /* taken from r3.out.ascii: but modified x and y order
      MN 1/2001. Now results comparable to r3.showdspf but
      for loops are different to r3.out.ascii and r3.to.sites - hmpf */

   /*AV*/
   /* IT WORKS WHIT A PARTICULAR FOR LOOP PROBABLY BECAUSE THE DATA
      ARE NOT STORED IN A 3D MATRIX [z,y,x] BUT IN A POINTER
      MANAGED AS (z,x,y) */

   for (z = 0; z < depths; z++) {
     G_percent(z, depths, 1);
     for (x = 0; x < cols; x++) {
       for (y = 0; y < rows; y++) {  /* north to south */

	 G3d_getValueRegion (map, x, y, z, d1p, typeIntern);

	 if (typeIntern == FCELL_TYPE) {
	   if (G3d_isNullValueNum(f1p, FCELL_TYPE)) {
	     g[cnt] = MISSING;
	     cnt++;
	   }
	   else {
	     g[cnt] = *f1p;
	     cnt++;
	   }
	 }
	 else { /*double*/
	   if (G3d_isNullValueNum(d1p, DCELL_TYPE)){
	     g[cnt]= MISSING;
	     cnt++;
	   }
	   else {
	     g[cnt] = (float) *d1p;
	     cnt++;
	   }
	 }
       }
     }
   }

   /************/

   /* Create the output v5d file */

   /*AV*/
   if (!v5dCreate(fileout, NumTimes, NumVars, rows, cols, Nl, VarName, 
		  TimeStamp, DateStamp,CompressMode, Projection, ProjArgs, 
		  Vertical, VertArgs ))
     G_fatal_error(_("Error: couldn't create %s"), fileout);


   /* Write the v5d file */
   if (!v5dWrite(1,1, g ))
     G_fatal_error(_("Error while writing grid. Disk full?"));

   /* Close the v5d file */
   v5dClose();

}

/*---------------------------------------------------------------------------*/
/* Main function: open the input and output files, then call
 * G3dtoascii.
 */
int
main (int argc, char *argv[]) {
  char *input, *output;
  int decim;
  struct Flag *coords;
  int trueCoords;
  struct GModule *module;

  /* Initialize GRASS */
  G_gisinit(argv[0]);
  module = G_define_module();
  module->keywords = _("raster3d, voxel");
    module->description =
   _("Export of GRASS 3D raster map to 3-dimensional Vis5D file.");

  /* Get parameters from user */
  setParams();

  coords = G_define_flag ();
  coords->key = 'm';
  coords->description = _("Use map coordinates instead of xyz coordinates");

  /* Have GRASS get inputs */
  if (G_parser(argc, argv))
    exit(-1);

  /* Parse input parameters */
  getParams(&input, &output, &decim);

  trueCoords=coords->answer;

  if(NULL == G_find_grid3(input, ""))
    G3d_fatalError(_("Requested 3d raster map not found"));

  map = G3d_openCellOld(input, G_find_grid3(input, ""), G3D_DEFAULT_WINDOW,
			G3D_TILE_SAME_AS_FILE,
			G3D_NO_CACHE);
  if (map == NULL)
     G3d_fatalError(_("Error opening 3d raster map"));

  /* Use default region */
  /*  G3d_getRegionStructMap(map, &region); */
  /* Figure out the region from current settings:*/
    G3d_getWindow(&region);

  G_debug(3, "cols: %i rows: %i layers: %i\n", region.cols, region.rows, region.depths);

  convert(output, region.rows, region.cols, region.depths, trueCoords);

  /* Close files and exit */
  if (!G3d_closeCell (map))
    fatalError (_("Error closing 3d raster map"));

  map = NULL;
  return (0);
}

/*---------------------------------------------------------------------------*/
