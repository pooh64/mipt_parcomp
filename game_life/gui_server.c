#include "life_core.h"
#include "life_gui.h"

#include <stdlib.h>
#include <unistd.h>
#include <time.h>

int gui_server(life_field_t *field, int serv_sock)
{
	ssize_t rc;
	int conn_sock = accept(serv_sock, NULL, NULL);
	if (conn_sock < 0) {
		perror("Error: accept");
		return -1;
	}

	struct gui_server_info info = {
		.field_w = field->w,
		.field_h = field->h,
	};

	size_t field_buf_sz = sizeof(*field->buf) * info.field_w * info.field_h;

	rc = writen(conn_sock, &info, sizeof(info));
	if (rc != sizeof(info)) {
		fprintf(stderr, "Error: can't send info\n");
		goto failure;
	}
	rc = writen(conn_sock, field->buf, field_buf_sz);
	if (rc != field_buf_sz) {
		fprintf(stderr, "Error: can't send field\n");
		goto failure;
	}

	while (1) {
		life_field_display(field);
		refresh();
		rc = readn(conn_sock, field->buf, field_buf_sz);
		if (rc != field_buf_sz) {
			fprintf(stderr, "Error: can't read field\n");
			goto failure;
		}
		usleep(1024L * 64);
	}
failure:
	if (conn_sock >= 0)
		close(conn_sock);
	return 0;
}

int main(int argc, char **argv)
{
	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);
	int h, w;
	getmaxyx(stdscr, h, w);

	life_field_t field;
	life_field_init(&field, w / LIFE_TILE_SIZE, h / LIFE_TILE_SIZE);

#if 0
	life_tile_t tile;
	life_tile_from_str(&tile, tile_exploder_str);
	life_field_set_tile(&field, &tile, field.w/2, field.h/2);
#else
	life_field_rand(&field);
#endif

	int serv_sock;
	if ((serv_sock = setup_gui_socket()) < 0)
		return 1;

	while (1)
		gui_server(&field, serv_sock);

	endwin();
	return 0;
}
