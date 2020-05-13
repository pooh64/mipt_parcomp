#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "general.h"

int g_size, g_rank;

void routine(struct routine_in *in, struct routine_out *out)
{
	out->arr[0] = in->arr[0] + in->arr[1];
	out->arr[1] = out->arr[0] + 1;
	out->cf = 0;
	if (out->arr[0] > MAX_ROUTINE_INT) {
		out->arr[0] %= (MAX_ROUTINE_INT + 1);
		out->cf += 1;
	}
	if (out->arr[1] > MAX_ROUTINE_INT) {
		out->arr[1] %= (MAX_ROUTINE_INT + 1);
		out->cf += 2;
	}
}

void write_outfile(char *path, struct routine_out *out, size_t out_s)
{
	FILE *file = fopen(path, "w");
	char cur_cf = 0;
	for (size_t i = 0; i < out_s; ++i) {
		out[i].arr[0] = out[i].arr[cur_cf];
		cur_cf = (out->cf >> cur_cf) & 1;
	}
	if (cur_cf)
		fprintf(file, "1");
	for (size_t i = out_s - 1; ; --i) {
		fprintf(file, "%9.9d", out[i].arr[0]);
		if (i == 0) break;
	}
	fclose(file);
}

void read_inpfile(char *path, struct routine_in **in, size_t *in_s)
{
	FILE *file = fopen(path, "r");
	size_t sz;
	fscanf(file, "%zu", &sz);
	assert((sz % 9) == 0);
	sz = sz / 9;
	struct routine_in *in_ = malloc(sizeof(*in_) * sz);

	for (size_t i = sz; i > 0; --i)
		fscanf(file, "%9d", &(in_[i-1].arr[0]));
	for (size_t i = sz; i > 0; --i)
		fscanf(file, "%9d", &(in_[i-1].arr[1]));
	*in = in_;
	*in_s = sz;
}

void slave()
{
	int task_sz;
	MPI_Status status;
	MPI_Recv(&task_sz, sizeof(task_sz), MPI_CHAR, MPI_ANY_SOURCE,
			TAG_START, MPI_COMM_WORLD, &status);

	struct task_in *in = malloc(sizeof(*in) * task_sz);
	struct task_out *out = malloc(sizeof(*out) * task_sz);
	while (1) {
		MPI_Recv(in, sizeof(*in) * task_sz, MPI_CHAR, MPI_ANY_SOURCE,
				MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		if (status.MPI_TAG != TAG_TASK)
			break;
		for (size_t i = 0; i < task_sz; ++i) {
			routine(&in[i].data, &out[i].data);
			out[i].task_id = in[i].task_id;
		}
		MPI_Send(out, sizeof(*out) * task_sz, MPI_CHAR,
				status.MPI_SOURCE, TAG_TASK, MPI_COMM_WORLD);
	}
	free(in); free(out);
}

void master_start(int task_sz)
{
	for (int i = 0; i < g_size; ++i) {
		if (g_rank == i)
			continue;
		MPI_Send(&task_sz, sizeof(task_sz), MPI_CHAR, i, TAG_START,
				MPI_COMM_WORLD);
	}
}

void master_shutdown()
{
	int msg = 0;
	for (int i = 0; i < g_size; ++i) {
		if (g_rank == i)
			continue;
		MPI_Send(&msg, sizeof(msg), MPI_CHAR, i, TAG_EXIT,
				MPI_COMM_WORLD);
	}
}
