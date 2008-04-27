
/****************************************************************************
*
* MODULE:       r3.out.vtk  
*   	    	
* AUTHOR(S):    Original author 
*               Soeren Gebbert soerengebbert at gmx de
* 		27 Feb 2006 Berlin
* PURPOSE:      Converts 3D raster maps (G3D) into the VTK-Ascii format  
*
* COPYRIGHT:    (C) 2005 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*   	    	License (>=v2). Read the file COPYING that comes with GRASS
*   	    	for details.
*
*****************************************************************************/

#ifndef __R3_OUT_VTK_WRITE_DATA_H__
#define __R3_OUT_VTK_WRITE_DATA_H__

struct input_maps;

/*Write the point coordinates of type point (1) or celldata (0) */
void write_vtk_points(input_maps * in, FILE * fp, G3D_Region region, int dp,
		    int type, double scale);
/*Write the uGrid Cells */
void write_vtk_unstructured_grid_cells(FILE * fp, G3D_Region region);
/*Write the outputdata */
void write_vtk_data(FILE * fp, void *map, G3D_Region region, char *varname,
		  int dp);
/*Write the rgb voxel data to the output */
void write_vtk_rgb_data(void *map_r, void *map_g, void *map_b, FILE * fp,
			  const char *string, G3D_Region region, int dp);
/*Write the vector data to the output */
void write_vtk_vector_data(void *map_x, void *map_y, void *map_z, FILE * fp,
			const char *string, G3D_Region region, int dp);

#endif

