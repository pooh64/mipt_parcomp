#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "general.h"

#define TRACE fprintf(stderr, "trace: line: %d\n", __LINE__)


void start_async_task(struct routine_in *in_arr, struct task_out *out_arr,
		MPI_Request *request_arr, int task_id, int rank)
{
	struct task_in tin;
	tin.task_id = task_id;
	tin.data    = in_arr[task_id];
	MPI_Send(&tin, sizeof(tin), MPI_CHAR, rank, TAG_TASK,
			MPI_COMM_WORLD);
	MPI_Irecv(&out_arr[rank], sizeof(*out_arr), MPI_CHAR, rank, TAG_TASK,
			MPI_COMM_WORLD, &request_arr[rank]);
}

void dynamic_master(char *in_path, char *out_path)
{
	struct routine_in *in;
	struct routine_out *out;
	size_t n_blocks;
	read_inpfile(in_path, &in, &n_blocks);
	out = malloc(sizeof(*out) * n_blocks);

	MPI_Request *requests = malloc(sizeof(*requests) * g_size);
	requests[g_rank] = MPI_REQUEST_NULL;
	struct task_out *tout = malloc(sizeof(*tout) * g_size);
	master_start(1);

	int task_id = 0;
	for (int i = 0; i < g_size; ++i) {
		if (i == g_rank)
			continue;
		if (task_id < n_blocks)
			start_async_task(in, tout, requests, task_id++, i);
		else
			requests[i] = MPI_REQUEST_NULL;
	}

	while (1) {
		int n;
		MPI_Status status;
		MPI_Waitany(g_size, requests, &n, &status);
		if (n == MPI_UNDEFINED)
			break;
		out[tout[n].task_id] = tout[n].data;
		if (task_id < n_blocks)
			start_async_task(in, tout, requests, task_id++, n);
		else
			requests[n] = MPI_REQUEST_NULL;
	}

	master_shutdown();
	write_outfile(out_path, out, n_blocks);
	free(in); free(out); free(tout); free(requests);
}

int main(int argc, char **argv)
{
	MPI_Init(&argc, &argv);

	MPI_Comm_size(MPI_COMM_WORLD, &g_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &g_rank);

	double time_start = MPI_Wtime();

	if (g_rank == 0)
		dynamic_master(argv[1], argv[2]);
	else
		slave();

	if (g_rank == 0)
		printf("time: %lg\n", MPI_Wtime() - time_start);

	MPI_Finalize();
	return 0;
}
