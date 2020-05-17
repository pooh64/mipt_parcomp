#ifndef _LIFE_CORE_H_
#define _LIFE_CORE_H_

#include <stdint.h>

#define LIFE_TILE_SIZE 8
typedef struct life_tile {
	uint8_t buf[LIFE_TILE_SIZE][LIFE_TILE_SIZE];
} life_tile_t;

typedef struct life_field {
	int w, h;
	life_tile_t *buf;
} life_field_t;

void life_field_get_top_bound(life_field_t *in, uint8_t *top, int line);
void life_field_get_bot_bound(life_field_t *in, uint8_t *bot, int line);

void life_line_evolve(life_tile_t *in, uint8_t *top, uint8_t *bot,
		life_tile_t *out, int w);
void life_field_init(life_field_t *field, int tile_w, int tile_h);
void life_field_set_tile(life_field_t *field, life_tile_t *tile, int x, int y);
void life_field_rand(life_field_t *field);

#endif
