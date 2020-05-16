#include <curses.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>

#define LIFE_TILE_SIZE 8
typedef struct life_tile {
	uint8_t buf[LIFE_TILE_SIZE][LIFE_TILE_SIZE];
} life_tile_t;

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

typedef struct life_field {
	int w, h;
	life_tile_t *buf;
} life_field_t;

void life_field_init(life_field_t *field, int tile_w, int tile_h)
{
	field->w = tile_w;
	field->h = tile_h;
	size_t sz = sizeof(*field->buf) * tile_w * tile_h;
	field->buf = malloc(sz);
	memset(field->buf, 0, sz);
}

static inline
void life_field_set_tile(life_field_t *field, life_tile_t *tile, int x, int y)
{
	field->buf[field->w * y + x] = *tile;
}

void life_field_display(life_field_t *field)
{
	for (int y = 0; y < field->h; ++y) {
		for (int x = 0; x < field->w; ++x)
			life_tile_display(&field->buf[field->w*y + x], x, y);
	}
}


typedef struct life_tile_ext {
	uint8_t buf[LIFE_TILE_SIZE+2][LIFE_TILE_SIZE+2];
} life_tile_ext_t;

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

static inline
void life_field_get_bounds(life_field_t *in, uint8_t *top, uint8_t *bot,
		int line)
{
	life_tile_t *ptr;
	for (int i = 0; i < in->w; ++i) {
		for (int x = 0; x < LIFE_TILE_SIZE; ++x) {
			ptr = &in->buf[(line-1)*in->w + i];
			if (line != 0)
				top[x] = ptr->buf[LIFE_TILE_SIZE-1][x];
			else
				top[x] = 0;

			ptr = &in->buf[(line+1)*in->w + i];
			if (line != in->h - 1)
				bot[x] = ptr->buf[0][x];
			else
				bot[x] = 0;
		}
		top += LIFE_TILE_SIZE;
		bot += LIFE_TILE_SIZE;
	}
}


void life_field_evolve(life_field_t *in, life_field_t *out)
{
	int line_len = sizeof(uint8_t) * in->w * LIFE_TILE_SIZE;
	uint8_t *top = malloc(line_len);
	uint8_t *bot = malloc(line_len);

	for (int y = 0; y < in->h; ++y) {
		life_field_get_bounds(in, top, bot, y);
		life_line_evolve(&in->buf[y * in->w], top, bot, &out->buf[y * in->w], in->w);
	}
	free(top);
	free(bot);
}

int main(int argc, char **argv)
{
	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);

	int h, w;
	getmaxyx(stdscr, h, w);

	life_tile_t tile;
	life_tile_from_str(&tile, tile_exploder_str);
	life_field_t field[2];
	life_field_init(&field[0], w / LIFE_TILE_SIZE, h / LIFE_TILE_SIZE);
	life_field_init(&field[1], w / LIFE_TILE_SIZE, h / LIFE_TILE_SIZE);
	life_field_set_tile(&field[0], &tile, field[0].w/2, field[0].h/2);

	life_field_t *cur = &field[0], *tmp = &field[1], *swp;
	for (int frame = 0; frame < 20000; ++frame) {
		life_field_display(cur);
		refresh();
		usleep(1024L * 256);
		life_field_evolve(cur, tmp);
		swp = cur; cur = tmp; tmp = swp;
	}

	endwin();
	return 0;
}
