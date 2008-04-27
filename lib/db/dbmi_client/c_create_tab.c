#include <grass/dbmi.h>
#include "macros.h"

/*!
 \fn int db_create_table (dbDriver *driver, dbTable *table)
 \brief 
 \return 
 \param 
*/
int
db_create_table (dbDriver *driver, dbTable *table)
{
    int ret_code;

/* start the procedure call */
    db__set_protocol_fds (driver->send, driver->recv);
    DB_START_PROCEDURE_CALL(DB_PROC_CREATE_TABLE);

/* send the argument(s) to the procedure */
    DB_SEND_TABLE_DEFINITION (table);

/* get the return code for the procedure call */
    DB_RECV_RETURN_CODE(&ret_code);

    if (ret_code != DB_OK)
	return ret_code; /* ret_code SHOULD == DB_FAILED */

/* no results */
    return DB_OK;
}
