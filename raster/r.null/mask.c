#include <grass/gis.h>
#include "mask.h"
#include "local_proto.h"

int init_d_mask_rules (d_Mask *d_mask)
{
    d_mask->list = NULL;

    return 0;
}

int add_d_mask_rule (d_Mask *d_mask, double a, double b, int inf)
{
    d_Interval *I;

    I = (d_Interval *) G_malloc (sizeof(d_Interval));
    I->low  = a <= b ? a : b ;
    I->high = a >= b ? a : b ;
    I->inf  = inf;
    I->next = d_mask->list;
    d_mask->list = I;

    return 0;
}

int mask_raster_array (void *rast, int ncols,
    int change_null, RASTER_MAP_TYPE data_type)
{
    DCELL x;

    while (ncols-- > 0)
    {
        x = G_get_raster_value_d(rast, data_type);
	if(change_null && G_is_null_value(rast, data_type))
	    G_set_raster_value_d(rast, new_null, data_type);
	if (mask_d_select (&x, &d_mask))
	    G_set_null_value(rast, 1, data_type);
        rast = G_incr_void_ptr(rast, G_raster_size(data_type));
    }

    return 0;
}

int mask_d_select (DCELL *x, d_Mask *mask)
{
    d_Interval *I;

    if (mask->list == NULL) return 0;
    for (I = mask->list; I; I = I->next)
    {
	if (mask_match_d_interval (*x, I))
	    return 1;
    }
    return 0;
}

int mask_match_d_interval (DCELL x, d_Interval *I)
{
    if (G_is_d_null_value(&x))
	return 0;

    if (I->inf < 0)
	return x <= I->low;

    if (I->inf > 0)
	return x >= I->high;

    return x >= I->low && x <= I->high;
}
