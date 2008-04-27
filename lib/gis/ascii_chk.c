/**
 * \file ascii_chk.c
 *
 * \brief
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 1999-2006
 */

#include <grass/gis.h>


#define TAB	011
#define SPACE	040


/**
 * \fn int G_ascii_check (char *string)
 *
 * \brief Removes non-ascii characters from buffer.
 *
 * Updates <b>string</b> with non_ascii characters removed, except for 
 * tabs, which are turned into spaces.
 *
 * \param[in,out] string buffer to have non-ascii characters removed
 * \return always returns 0
 */

int G_ascii_check (char *string)
{
    char *ptr1, *ptr2 ;

    ptr1 = string ;
    ptr2 = string ;

    while(*ptr1)
    {
	if ((*ptr1 >= 040) && (*ptr1 <= 0176))
	    *ptr2++ = *ptr1 ;
	else if (*ptr1 == TAB)
	    *ptr2++ = SPACE ;
	ptr1++ ;
    }
    *ptr2 = 0 ;

    return 0;
}
