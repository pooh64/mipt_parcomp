#include <mpi.h>

#include "life_core.h"
#include "life_gui.h"

#include <stdlib.h>

#define TRACE(fmt, ...) printf("[%.2d]: %d: " fmt "\n",	\
		g_rank, __LINE__, ##__VA_ARGS__)

int g_size, g_rank;

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

struct slave_info {
	int field_w, field_h;
	int rank_prev, rank_next;
};

void slave_bound_exchange(int rank_prev, int rank_next, life_field_t *fcur,
		uint8_t *first_bound, uint8_t *last_bound, size_t bound_sz)
{
	if (rank_prev != -1) {
		MPI_Recv(first_bound, bound_sz, MPI_CHAR, rank_prev, MPI_ANY_TAG, MPI_COMM_WORLD, NULL);
		life_field_get_bot_bound(fcur, last_bound, -1);
		MPI_Send(last_bound, bound_sz, MPI_CHAR, rank_prev, 0, MPI_COMM_WORLD);
	}
	if (rank_next != -1) {
		life_field_get_top_bound(fcur, last_bound, fcur->h);
		MPI_Send(last_bound, bound_sz, MPI_CHAR, rank_next, 0, MPI_COMM_WORLD);
		MPI_Recv(last_bound, bound_sz, MPI_CHAR, rank_next, MPI_ANY_TAG, MPI_COMM_WORLD, NULL);
	}
}

void slave()
{
	struct slave_info info;
	TRACE("slave receive info");
	MPI_Recv(&info, sizeof(info), MPI_CHAR, 0, MPI_ANY_TAG, MPI_COMM_WORLD, NULL);
	TRACE("slave info received");

	life_field_t field[2], *fcur, *ftmp, *fswp;
	life_field_init(&field[0], info.field_w, info.field_h);
	life_field_init(&field[1], info.field_w, info.field_h);
	size_t field_sz = sizeof(*field[0].buf) * field[0].w * field[0].h;
	TRACE("slave receive field");
	MPI_Recv(field[0].buf, field_sz, MPI_CHAR, 0, MPI_ANY_TAG, MPI_COMM_WORLD, NULL);
	TRACE("slave field received");

	size_t bound_sz = LIFE_TILE_SIZE * field[0].w;
	uint8_t *first_bound = malloc(bound_sz);
	uint8_t  *last_bound = malloc(bound_sz);
	uint8_t *top = calloc(bound_sz, 1);
	uint8_t *bot = calloc(bound_sz, 1);

	fcur = &field[0]; ftmp = &field[1];
	while (1) {
		slave_bound_exchange(info.rank_prev, info.rank_next, fcur,
				first_bound, last_bound, bound_sz);
		for (int y = 0; y < fcur->h; ++y) {
			uint8_t *top_tmp, *bot_tmp;
			if (y == 0)
				top_tmp = first_bound;
			else
				life_field_get_top_bound(fcur, (top_tmp = top), y);
			if (y == fcur->h - 1)
				bot_tmp = last_bound;
			else
				life_field_get_bot_bound(fcur, (bot_tmp = bot), y);
			life_line_evolve(&fcur->buf[y * fcur->w], top_tmp, bot_tmp,
					&ftmp->buf[y * fcur->w], fcur->w);
		}
		MPI_Send(fcur->buf, field_sz, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
		fswp = ftmp; ftmp = fcur; fcur = fswp;
	}
}

void master_init(life_field_t *field, struct slave_info **info_p)
{
	struct slave_info *info = malloc(sizeof(*info) * g_size);
	*info_p = info;
	int n_lines = field->h;
	life_tile_t *ptr = field->buf;
	for (int i = 1; i < g_size; ++i) {
		info[i].rank_prev = (i == 1 ? -1 : i-1);
		info[i].rank_next = (i == g_size-1 ? -1 : i+1);
		info[i].field_w = field->w;
		info[i].field_h = n_lines / (g_size - i);
		MPI_Send(&info[i], sizeof(info[i]), MPI_CHAR, i, 0, MPI_COMM_WORLD);
		MPI_Send(ptr, info[i].field_h * info[i].field_w * sizeof(*ptr),
				MPI_CHAR, i, 0, MPI_COMM_WORLD);
		n_lines -= info[i].field_h;
		ptr += info[i].field_h * info[i].field_w;
	}
}

void master_collect(life_field_t *field, struct slave_info *info)
{
	life_tile_t *ptr = field->buf;
	for (int i = 1; i < g_size; ++i) {
		MPI_Recv(ptr, sizeof(*ptr) * info[i].field_h * info[i].field_w,
				MPI_CHAR, i, MPI_ANY_TAG, MPI_COMM_WORLD, NULL);
		ptr += info[i].field_h * info[i].field_w;
	}
}

void master()
{
	if (g_size == 1) {
		TRACE("no slaves in process!");
		return;
	}
	life_field_t *field;
	ssize_t rc, field_buf_sz;

	int sock = setup_worker_socket();
	if (sock < 0) {
		int h = 1024, w = 1024;
		life_field_init(&field, w / LIFE_TILE_SIZE, h / LIFE_TILE_SIZE);
		life_field_rand(&field);
	} else {
		struct gui_server_info;
		rc = readn(sock, &info, sizeof(info));
		if (rc != sizeof(info)) {
			TRACE("Error: can't receive gui info");
			MPI_Abort(1);
		}
		field_buf_sz = sizeof(*field->buf) * info.field_w * info.field_h;
		life_field_init(&field, info.field_w, info.field_h);
		rc = readn(sock, field->buf, field_buf_sz);
		if (rc != field_buf_sz) {
			TRACE("Error: can't receive gui field");
			MPI_Abort(1);
		}
	}
	int cl_frames = 0;
	float cl = MPI_Wtime();

	struct slave_info *info;
	TRACE("master_init");
	master_init(&field, &info);
	TRACE("master_init completed");

	while (1) {
		master_collect(&field, info);
		if (++cl_frames == 128) {
			TRACE("fps: %g", cl_frames / (MPI_Wtime() - cl));
			cl = MPI_Wtime();
			cl_frames = 0;
		}
		if (sock >= 0) {
			rc = writen(sock, field->buf, field_buf_sz);
			if (rc != field_buf_sz) {
				TRACE("Error: can't send gui field");
				MPI_Abort(1);
			}
		}
	}
}


int main(int argc, char **argv)
{
	MPI_Init(&argc, &argv);

	MPI_Comm_rank(MPI_COMM_WORLD, &g_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &g_size);

	if (g_rank == 0)
		master();
	else
		slave();

	MPI_Finalize();
	return 0;
}
