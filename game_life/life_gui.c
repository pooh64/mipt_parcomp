#include <curses.h>
#include <string.h>
#include "life_gui.h"

char *tile_glider_str =
"        "
"    #   "
"  # #   "
"   ##   "
"        "
"        "
"        "
"        ";

char *tile_exploder_str =
"        "
" # # #  "
" #   #  "
" #   #  "
" #   #  "
" # # #  "
"        "
"        ";

void life_tile_from_str(life_tile_t *tile, char *str)
{
	strncpy((char*) tile, str, sizeof(*tile));
	size_t len = strlen(str);
	for (size_t i = 0; i < len && i < sizeof(*tile); ++i)
		((char*) (*tile->buf))[i] = (str[i] == '#') ? 1 : 0;
}

static inline
void life_tile_display(life_tile_t *tile, int tile_x, int tile_y)
{
	tile_x *= (LIFE_TILE_SIZE);
	tile_y *= (LIFE_TILE_SIZE);

	for (uint8_t y = 0; y < LIFE_TILE_SIZE; ++y) {
		for (uint8_t x = 0; x < LIFE_TILE_SIZE; ++x) {
			uint8_t ch = tile->buf[y][x];
			ch = ch ? '#' : ' ';
			mvaddch(tile_y + y, tile_x + x, ch);
		}
	}
}

void life_field_display(life_field_t *field)
{
	for (int y = 0; y < field->h; ++y) {
		for (int x = 0; x < field->w; ++x)
			life_tile_display(&field->buf[field->w*y + x], x, y);
	}
}