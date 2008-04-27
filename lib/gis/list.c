/**
   \file list.c

   \brief List elements

   \author Unknown (probably CERL)

   (C) 2000 by the GRASS Development Team

   This program is free software under the GNU General Public
   License (>=v2). Read the file COPYING that comes with GRASS
   for details.
*/

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>

#include <grass/gis.h>
#include <grass/glocale.h>

static int broken_pipe;
static int hit_return = 0;
static int list_element(FILE *,const char *,const char *,const char *,
			int (*)(const char *, const char *, const char *));
static void sigpipe_catch(int);

int G_set_list_hit_return(int flag)
{
    hit_return = flag;
    return 0;
}

/**
   \brief General purpose list function.

   Will list files from all mapsets
   in the mapset list for a specified database element.

   Note: output is to stdout piped thru the more utility

   lister (char *name char *mapset, char* buf)
   
   Given file 'name', and 'mapset', lister() should copy a string into 'buf'
   when called with name == "", should set buf to general title for mapset list.

   \param element    Database element (eg, "cell", "cellhd", etc)
   \param desc       Description for element (if NULL, element is used)
   \param mapset     Mapset to be listed "" to list all mapsets in mapset search list 
   "." will list current mapset
   \param lister     If given will call this routine to get a list
   title. NULL if no titles desired. 

   \return Number of elements
*/
int G_list_element (
    const char *element,
    const char *desc,
    const char *mapset,
    int (*lister)(const char *, const char *, const char *))
{
    int n;
    FILE *more;
    int count;
#ifdef SIGPIPE
    void (*sigpipe)();
#endif

/* must catch broken pipe in case "more" quits */
    broken_pipe = 0;
#ifdef SIGPIPE
    sigpipe = signal (SIGPIPE, sigpipe_catch);
#endif

    count = 0;
    if (desc == 0 || *desc == 0)
	desc = element;

    /* G_popen() does not work with MinGW? */
#ifndef __MINGW32__
/*
 * G_popen() the more command to page the output
 */
    if (isatty(1))
    {
#ifdef __MINGW32__
	more = G_popen ("%GRASS_PAGER%","w");
#else
	more = G_popen ("$GRASS_PAGER","w");
#endif
	if (!more) more = stdout;
    }
    else
#endif
	more = stdout;
    fprintf (more,"----------------------------------------------\n");

/*
 * if no specific mapset is requested, list the mapsets
 * from the mapset search list
 * otherwise just list the specified mapset
 */
    if (mapset == 0 || *mapset == 0)
	for (n = 0; !broken_pipe && (mapset = G__mapset_name (n)); n++)
	    count += list_element (more, element, desc, mapset, lister);
    else
	count += list_element (more, element, desc, mapset, lister);

    if (!broken_pipe)
    {
	if (count == 0){
	   if (mapset == 0 || *mapset == 0)
	    fprintf (more,_("no %s files available in current mapset\n"), desc);
	   else
	    fprintf (more,_("no %s files available in mapset <%s>\n"), desc, mapset);
	}

	fprintf (more,"----------------------------------------------\n");
    }
/*
 * close the more
 */
    if (more != stdout) G_pclose (more);
#ifdef SIGPIPE
    signal (SIGPIPE, sigpipe);
#endif
if (hit_return && isatty(1))
    {
	fprintf (stderr, _("hit RETURN to continue -->"));
	while (getchar() != '\n')
	    ;
    }

    return 0;
}

static void sigpipe_catch(int n)
{
    broken_pipe = 1;
    signal (n,sigpipe_catch);
}

static int list_element( FILE *out,
    const char *element, const char *desc, const char *mapset,
    int (*lister)(const char *, const char *, const char *))
{
    char path[GPATH_MAX];
    int count = 0;
    const char **list;
    int i;

/*
 * convert . to current mapset
 */
    if (strcmp (mapset,".") == 0)
	mapset = G_mapset();


/*
 * get the full name of the GIS directory within the mapset
 * and list its contents (if it exists)
 *
 * if lister() routine is given, the ls command must give 1 name
 */
    G__file_name (path, element, "", mapset);
    if (access(path, 0) != 0)
    {
	fprintf(out,"\n");
    	return count;
    }

/*
 * if a title so that we can call lister() with the names
 * otherwise the ls must be forced into columnar form.
 */

    list = G__ls(path, &count);

    if (count > 0)
    {
        fprintf(out, _("%s files available in mapset <%s>:\n"), desc, mapset);
        if (lister)
        {
    	    char title[400];
    	    char name[GNAME_MAX];

    	    *name = *title = 0;
    	    lister (name, mapset, title);
    	    if (*title)
    	        fprintf(out,"\n%-18s %-.60s\n",name,title);
        }
    }

    if (lister)
    {
        for(i = 0; i < count; i++)
        {
	    char title[400];
    
            lister (list[i], mapset, title);
            fprintf(out,"%-18s %-.60s\n",list[i],title);
	}
    }
    else
        G_ls_format(list, count, 0, out);

    fprintf(out, "\n");

    for(i = 0; i < count; i++)       
	G_free((char *)list[i]);
    if (list)
        G_free(list);

    return count;
}

/*!
 * \brief List specified type of elements. Application must release
          the allocated memory.
 * \param element Element type (G_ELEMENT_RASTER, G_ELEMENT_VECTOR, 
                                G_ELEMENT_REGION )
 * \param gisbase Path to GISBASE
 * \param location Location name
 * \param mapset Mapset name
 * \return Zero terminated array of element names
 */
char **G_list(int element, const char *gisbase, const char *location, const char *mapset)
{
    char *el;
    char *buf;
    DIR *dirp;
    struct dirent *dp;
    int count;
    char **list;
    
    switch ( element )
    {
        case G_ELEMENT_RASTER:
            el = "cell";
            break;

        case G_ELEMENT_GROUP:
            el = "group";
            break;

        case G_ELEMENT_VECTOR:
            el = "vector";
            break;

        case G_ELEMENT_REGION:
            el = "windows";
            break;

        default:
            G_fatal_error (_("G_list: Unknown element type"));
    }			
	
    buf = (char *) G_malloc ( strlen(gisbase) + strlen(location)
                              + strlen(mapset) + strlen(el) + 4 );

    sprintf ( buf, "%s/%s/%s/%s", gisbase, location, mapset, el );

    dirp = opendir(buf);
    G_free ( buf );		

    if ( dirp == NULL ) /* this can happen if element does not exist */
    {
        list = (char **) G_calloc ( 1, sizeof(char *) );
        return list;
    }

    count = 0;
    while((dp = readdir(dirp)) != NULL)
    {
	if (dp->d_name[0] == '.') continue;
        count++;
    }
    rewinddir(dirp);

    list = (char **) G_calloc ( count+1, sizeof(char *) );
    
    count = 0;
    while ((dp = readdir(dirp)) != NULL)
    {
        if (dp->d_name[0] == '.') continue;

        list[count] = (char *) G_malloc( strlen(dp->d_name)+1 );
        strcpy ( list[count], dp->d_name );
        count++;
    }
    closedir(dirp);

    return list;
}

/**
   \brief Free list
   
   \param list char* array to be freed

   \return
*/
void G_free_list(char **list)
{
    int i = 0;

    if ( !list ) return;
   
    while ( list[i] )
    {
        G_free (list[i]);
        i++;
    }
    G_free (list);
}
