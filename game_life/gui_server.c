#include "life_core.h"
#include "life_gui.h"

#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <errno.h>
#include <signal.h>

#define SOCKET_PATH "/tmp/life_gui.sock"

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

struct gui_server_info {
	int field_w, field_h;
};

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
