#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "life_core.h"

typedef struct life_tile_ext {
	uint8_t buf[LIFE_TILE_SIZE+2][LIFE_TILE_SIZE+2];
} life_tile_ext_t;

void life_field_init(life_field_t *field, int tile_w, int tile_h)
{
	field->w = tile_w;
	field->h = tile_h;
	size_t sz = sizeof(*field->buf) * tile_w * tile_h;
	field->buf = malloc(sz);
	memset(field->buf, 0, sz);
}

void life_field_set_tile(life_field_t *field, life_tile_t *tile, int x, int y)
{
	field->buf[field->w * y + x] = *tile;
}

static inline void
life_tile_to_ext(life_tile_t *in, uint8_t *top, uint8_t *bot, int w, int id,
		life_tile_ext_t *ext)
{
	top += id * LIFE_TILE_SIZE;
	bot += id * LIFE_TILE_SIZE;

	for (int y = 1; y < LIFE_TILE_SIZE + 1; ++y) {
		for (int x = 1; x < LIFE_TILE_SIZE + 1; ++x)
			ext->buf[y][x] = in[id].buf[y-1][x-1];
	}

	for (int x = 1; x < LIFE_TILE_SIZE + 1; ++x) {
		ext->buf[0][x] = top[x-1];
		ext->buf[LIFE_TILE_SIZE+1][x] = bot[x-1];
	}

	if (id == 0) {
		for (int y = 0; y < LIFE_TILE_SIZE + 2; ++y)
			ext->buf[y][0] = 0;
	} else {
		ext->buf[0][0] = top[-1];
		ext->buf[LIFE_TILE_SIZE+1][0] = bot[-1];
		for (int y = 1; y < LIFE_TILE_SIZE+1; ++y)
			ext->buf[y][0] = in[id-1].buf[y-1][LIFE_TILE_SIZE-1];
	}

	if (id == w - 1) {
		for (int y = 0; y < LIFE_TILE_SIZE + 2; ++y)
			ext->buf[y][LIFE_TILE_SIZE+1] = 0;
	} else {
		ext->buf[0][LIFE_TILE_SIZE+1] = top[LIFE_TILE_SIZE];
		ext->buf[LIFE_TILE_SIZE+1][LIFE_TILE_SIZE+1] = bot[LIFE_TILE_SIZE];
		for (int y = 1; y < LIFE_TILE_SIZE+1; ++y)
			ext->buf[y][LIFE_TILE_SIZE+1] = in[id+1].buf[y-1][0];
	}
}

static inline
void life_tile_ext_evolve(life_tile_ext_t *in, life_tile_t *out)
{
	memset(out, 0, sizeof(*out));
	for (int y = 1; y < LIFE_TILE_SIZE+1; ++y) {
		for (int x = 1; x < LIFE_TILE_SIZE+1; ++x) {
			uint8_t sum = 0;
			for (int i = -1; i < 2; ++i) {
				for (int k = -1; k < 2; ++k)
					sum += in->buf[y+i][x+k];
			}
			if ((in->buf[y][x] && (sum == 3 || sum == 4)) ||
			    (!in->buf[y][x] && (sum == 3)))
				out->buf[y-1][x-1] = 1;
		}
	}
}

void life_line_evolve(life_tile_t *in, uint8_t *top, uint8_t *bot,
		life_tile_t *out, int w)
{
	life_tile_ext_t ext;
	for (int i = 0; i < w; ++i) {
		life_tile_to_ext(in, top, bot, w, i, &ext);
		life_tile_ext_evolve(&ext, &out[i]);
	}
}

void life_field_get_top_bound(life_field_t *in, uint8_t *top, int line)
{
	life_tile_t *ptr;
	for (int i = 0; i < in->w; ++i) {
		for (int x = 0; x < LIFE_TILE_SIZE; ++x) {
			ptr = &in->buf[(line-1)*in->w + i];
			if (line != 0)
				top[x] = ptr->buf[LIFE_TILE_SIZE-1][x];
			else
				top[x] = 0;
		}
		top += LIFE_TILE_SIZE;
	}
}

void life_field_get_bot_bound(life_field_t *in, uint8_t *bot, int line)
{
	life_tile_t *ptr;
	for (int i = 0; i < in->w; ++i) {
		for (int x = 0; x < LIFE_TILE_SIZE; ++x) {
			ptr = &in->buf[(line+1)*in->w + i];
			if (line != in->h - 1)
				bot[x] = ptr->buf[0][x];
			else
				bot[x] = 0;
		}
		bot += LIFE_TILE_SIZE;
	}
}

void life_field_rand(life_field_t *field)
{
	srand(time(0));
	size_t sz = field->w * field->h * sizeof(*field->buf);
	uint8_t *ptr = (uint8_t*) &field->buf[0];
	for (size_t i = 0; i < sz; ++i)
		ptr[i] = (rand() % 3 == 0) ? 1 : 0;
}
