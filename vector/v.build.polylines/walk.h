#define NO_CATS     0 
#define ONE_CAT     1
#define MULTI_CATS  2

int walk_back (struct Map_info*, int);
int walk_forward_and_pick_up_coords (struct Map_info*, int, struct line_pnts*, int *,
				     struct line_cats *, int);
