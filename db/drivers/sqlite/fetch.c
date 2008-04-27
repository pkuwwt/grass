/**
 * \file fetch.c
 *
 * \brief Low level SQLite database functions.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Radim Blazek
 *
 * \date 2005-2007
 */

#include <stdlib.h>
#include <string.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "globals.h"
#include "proto.h" 


/**
 * \fn int db__driver_fetch (dbCursor *cn, int position, int *more)
 *
 * \brief Low level SQLite database table record fetch.
 *
 * NOTE: <b>position</b> is one of:
 * DB_NEXT, DB_FIRST, DB_CURRENT, DB_PREVIOUS, DB_LAST.
 *
 * \param[in] cn open database cursor
 * \param[in] position database position. See NOTE.
 * \param[in,out] more 1 = more data; 0 = no more data
 * \return int
 */

int
db__driver_fetch (dbCursor *cn, int position, int *more)
{
    cursor     *c;
    dbToken    token;    
    dbTable    *table;
    int        i, ret;
    int        ns;

    /* get cursor token */
    token = db_get_cursor_token(cn);

    /* get the cursor by its token */
    if (!(c = (cursor *) db_find_token(token))) {
	append_error ("Cursor not found");
	report_error();
	return DB_FAILED;
    }

    G_debug ( 3, "fetch row = %d", c->row );

    /* fetch on position */
    switch (position)
    { 
	case DB_NEXT:
	case DB_FIRST:

            if ( position == DB_FIRST ) 
	        c->row = -1;

	    ret = sqlite3_step ( c->statement );
	    if ( ret != SQLITE_ROW )
	    {
		if ( ret != SQLITE_DONE ) 
		{
		    append_error ("Cannot step:\n");
		    append_error ((char *) sqlite3_errmsg (sqlite));
		    report_error();
		    return DB_FAILED;
		}
		sqlite3_reset ( c->statement );
		*more = 0;
		return DB_OK;
	    }
	    c->row++;
	    break;

	case DB_CURRENT:
	    break;

	case DB_PREVIOUS:
	    append_error ("DB_PREVIOUS is not supported");
	    report_error();
	    return DB_FAILED;
	    break;

	case DB_LAST:
	    append_error ("DB_LAST is not supported");
	    report_error();
	    return DB_FAILED;
	    break;
    };

    *more = 1;

    /* get the data out of the descriptor into the table */
    table = db_get_cursor_table(cn);

    for (i = 0; i < c->nkcols; i++) 
    {
	int col, litetype, sqltype;
	dbColumn   *column;
	dbValue    *value;

	col = c->kcols[i]; /* known cols */
 		
	column = db_get_table_column (table, i);
	sqltype = db_get_column_sqltype(column);
/*	fails for dates: 
        litetype  = db_get_column_host_type(column); 
*/
	litetype = sqlite3_column_type ( c->statement, col );

	value  = db_get_column_value (column);
	db_zero_string (&value->s);
	
	/* Is null? */
	if ( sqlite3_column_type(c->statement, col) == SQLITE_NULL ) {
	    value->isNull = 1;
	    continue;
	} else {
	    value->isNull = 0;
	}

	G_debug (3, "col %d, litetype %d, sqltype %d: val = '%s'", 
		    col, litetype, sqltype, 
		    sqlite3_column_text ( c->statement, col) );

       /* http://www.sqlite.org/capi3ref.html#sqlite3_column_type
           SQLITE_INTEGER  1
           SQLITE_FLOAT    2
           SQLITE_TEXT     3
           SQLITE_BLOB     4
           SQLITE_NULL     5
        */

	/* Note: we have set DATESTYLE TO ISO in db_driver_open_select_cursor() so datetime
	 *       format should be ISO */

	switch ( litetype ) {
	    case SQLITE_TEXT:
		if (sqltype == 6 ) { /* date string */
		   /* Example: '1999-01-25' */
		   G_debug(3, "sqlite fetched date: %s",sqlite3_column_text ( c->statement, col));
		   ns = sscanf((char *) sqlite3_column_text ( c->statement, col), "%4d-%2d-%2d",
                             &(value->t.year), &(value->t.month), &(value->t.day) );
		   if ( ns != 3 ) {
			append_error ( "Cannot scan date:");
			append_error ((char *) sqlite3_column_text (c->statement, col));
			report_error();
			return DB_FAILED;
                   }
                   value->t.hour = 0;
                   value->t.minute = 0;
                   value->t.seconds = 0.0;
		} else { /* other string */
		    db_set_string ( &(value->s), (char *) sqlite3_column_text ( c->statement, col));
		}
		break;

	    case SQLITE_INTEGER:
	    	value->i = sqlite3_column_int ( c->statement, col);
		break;
		
	    case SQLITE_FLOAT:
	    	value->d = sqlite3_column_double ( c->statement, col);
		break;

	    case SQLITE_NULL:
		/* do nothing  ? */
		break;
	}
    }

    G_debug (3, "Row fetched" );

    return DB_OK;
}


/**
 * \fn int db__driver_get_num_rows (dbCursor *cn)
 *
 * \brief Gets number of rows in SQLite database table.
 *
 * \param[in] cn open database cursor
 * \return int number of rows in table
 */

int
db__driver_get_num_rows  (dbCursor *cn)

{
    cursor     *c;
    dbToken    token;
    int row;

    /* get cursor token */
    token = db_get_cursor_token(cn);

    /* get the cursor by its token */
    if (!(c = (cursor *) db_find_token(token))) {
       append_error("Cursor not found");
       report_error();
       return DB_FAILED;
    }

    if ( c->nrows > -1 ) 
    {
	return ( c->nrows );
    }

    sqlite3_reset ( c->statement );

    c->nrows = 0;
    while ( sqlite3_step ( c->statement ) == SQLITE_ROW )
    {
        c->nrows++;
    }

    sqlite3_reset ( c->statement );

    /* Reset cursor position */
    row = -1;
    if ( c->row > -1 ) 
    {
        while ( sqlite3_step ( c->statement ) == SQLITE_ROW )
        {
            if ( row == c->row )
		break;
	
  	    row++;
        }
    }

    return ( c->nrows );
}
