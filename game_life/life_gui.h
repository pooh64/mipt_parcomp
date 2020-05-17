#ifndef _LIFE_GUI_H_
#define _LIFE_GUI_H_

#include <curses.h>
#include "life_core.h"

extern char *tile_glider_str;
extern char *tile_exploder_str;

void life_tile_from_str(life_tile_t *tile, char *str);
void life_field_display(life_field_t *field);

#endif
