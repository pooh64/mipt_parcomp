#include "life_core.h"
#include "life_gui.h"

#include <stdlib.h>
#include <unistd.h>
#include <time.h>

void life_field_evolve(life_field_t *in, life_field_t *out)
{
	int line_len = sizeof(uint8_t) * in->w * LIFE_TILE_SIZE;
	uint8_t *top = malloc(line_len);
	uint8_t *bot = malloc(line_len);

	for (int y = 0; y < in->h; ++y) {
		life_field_get_top_bound(in, top, y);
		life_field_get_bot_bound(in, bot, y);
		life_line_evolve(&in->buf[y * in->w], top, bot, &out->buf[y * in->w], in->w);
	}
	free(top);
	free(bot);
}

int main(int argc, char **argv)
{
#ifdef LIFE_ENABLE_GUI
	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);
	int h, w;
	getmaxyx(stdscr, h, w);
#else
	int h = 1024, w = 1024;
	clock_t cl = clock();
	int cl_frames = 0;
#endif

	life_field_t field[2];
	life_field_init(&field[0], w / LIFE_TILE_SIZE, h / LIFE_TILE_SIZE);
	life_field_init(&field[1], w / LIFE_TILE_SIZE, h / LIFE_TILE_SIZE);

#ifdef EXPLODER
	life_tile_t tile;
	life_tile_from_str(&tile, tile_exploder_str);
	life_field_set_tile(&field[0], &tile, field[0].w/2, field[0].h/2);
#else
	life_field_rand(&field[0]);
#endif
	life_field_t *cur = &field[0], *tmp = &field[1], *swp;
	for (int frame = 0; frame < 10000; ++frame) {
#ifdef LIFE_ENABLE_GUI
		life_field_display(cur);
		refresh();
		usleep(1024L * 128);
#else
		if (cl_frames++ == 64) {
			cl = clock() - cl;
			cl_frames--;
			printf("fps: %g\n", CLOCKS_PER_SEC * cl_frames / (float) cl);
			cl_frames = 0;
			cl = clock();
		}
#endif
		life_field_evolve(cur, tmp);
		swp = cur; cur = tmp; tmp = swp;
	}

#ifdef LIFE_ENABLE_GUI
	endwin();
#endif
	return 0;
}
