#include <curses.h>
#include <string.h>
#include <stdlib.h>
#include "life_gui.h"

#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <errno.h>
#include <signal.h>

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

ssize_t writen(int fd, void *buf, size_t size)
{
	char *ptr = buf;
	size_t start_size = size;
	while (size) {
		ssize_t ret = write(fd, ptr, size);
		if (ret == 0)
			return -1;
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		size -= ret;
		ptr += ret;
	}
	return start_size;
}

ssize_t readn(int fd, void *buf, size_t size)
{
	char *ptr = buf;
	size_t start_size = size;
	while (size) {
		ssize_t ret = read(fd, ptr, size);
		if (ret == 0)
			return start_size - size;
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		size -= ret;
		ptr += ret;
	}
	return start_size;
}

int setup_gui_socket()
{
	struct sigaction sa_ignore = {
		.sa_handler = SIG_IGN
	};
	if (sigaction(SIGPIPE, &sa_ignore, NULL) < 0) {
		perror("Error: sigaction");
		return -1;
	}

	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	addr.sun_path[0] = '\0';
	strncpy(&addr.sun_path[1], SOCKET_PATH, sizeof(addr.sun_path) - 2);

	int sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("Error: socket");
		return -1;
	}

	if (bind(sock, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
		if (errno == EADDRINUSE)
			perror("Error: socket already in use");
		else
			perror("Error: bind");
		return -1;
	}

	if (listen(sock, 127) < 0) {
		perror("Error: listen\n");
		return -1;
	}

	return sock;
}

int setup_worker_socket()
{
	struct sigaction sa_ignore = {
		.sa_handler = SIG_IGN
	};
	if (sigaction(SIGPIPE, &sa_ignore, NULL) < 0) {
		perror("Error: sigaction");
		return -1;
	}

	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	addr.sun_path[0] = '\0';
	strncpy(&addr.sun_path[1], SOCKET_PATH, sizeof(addr.sun_path) - 2);

	int sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("Error: socket");
		return -1;
	}

	if (connect(sock, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
		perror("Error: connect");
		return -1;
	}

	return sock;
}

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

	start_color();
	init_pair(1, COLOR_RED, COLOR_BLACK);
	init_pair(2, COLOR_BLACK, COLOR_WHITE);

	attron(COLOR_PAIR(1));
	for (uint8_t y = 0; y < LIFE_TILE_SIZE; ++y) {
		for (uint8_t x = 0; x < LIFE_TILE_SIZE; ++x)
			mvaddch(tile_y + y, tile_x + x, ' ');
	}
	attroff(COLOR_PAIR(1));

	attron(COLOR_PAIR(2));
	for (uint8_t y = 0; y < LIFE_TILE_SIZE; ++y) {
		for (uint8_t x = 0; x < LIFE_TILE_SIZE; ++x) {
			if (tile->buf[y][x])
				mvaddch(tile_y + y, tile_x + x, '0');
		}
	}
	attroff(COLOR_PAIR(2));
}

void life_field_display(life_field_t *field)
{
	for (int y = 0; y < field->h; ++y) {
		for (int x = 0; x < field->w; ++x)
			life_tile_display(&field->buf[field->w*y + x], x, y);
	}
}
