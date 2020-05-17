#ifndef _LIFE_GUI_H_
#define _LIFE_GUI_H_

#include <curses.h>
#include "life_core.h"

#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <errno.h>
#include <signal.h>

extern char *tile_glider_str;
extern char *tile_exploder_str;

struct gui_server_info {
	int field_w, field_h;
};

#define SOCKET_PATH "/tmp/life_gui.sock"

ssize_t writen(int fd, void *buf, size_t size);
ssize_t readn(int fd, void *buf, size_t size);
int setup_gui_socket();
int setup_worker_socket();

void life_tile_from_str(life_tile_t *tile, char *str);
void life_field_display(life_field_t *field);

#endif
