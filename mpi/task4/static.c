#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "general.h"

void static_master(char *in_path, char *out_path)
{
	struct routine_in *in;
	struct routine_out *out;
	size_t n_blocks;
	read_inpfile(in_path, &in, &n_blocks);
	out = malloc(sizeof(*out) * n_blocks);

	int task_sz = n_blocks / (g_size - 1);
	master_start(task_sz);

	struct task_in *tin = malloc(sizeof(*tin) * task_sz);
	struct task_out *tout = malloc(sizeof(*tout) * task_sz);
	int task_id = 0;

	for (int i = 0; task_id < n_blocks; ++i) {
		if (i == g_rank)
			continue;
		for (int n = 0; n < task_sz; ++n) {
			tin[n].task_id = task_id;
			tin[n].data    = in[task_id++];
		}
		MPI_Send(tin, sizeof(*tin) * task_sz, MPI_CHAR, i, TAG_TASK,
				MPI_COMM_WORLD);
	}

	for (int i = 0; i < g_size; ++i) {
		if (i == g_rank)
			continue;
		MPI_Recv(tout, sizeof(*tout) * task_sz, MPI_CHAR, i, TAG_TASK,
				MPI_COMM_WORLD, NULL);
		for (int n = 0; n < task_sz; ++n)
			out[tout[n].task_id] = tout[n].data;
	}

	master_shutdown();
	write_outfile(out_path, out, n_blocks);
	free(in); free(out); free(tin); free(tout);
}

int main(int argc, char **argv)
{
	MPI_Init(&argc, &argv);

	MPI_Comm_size(MPI_COMM_WORLD, &g_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &g_rank);

	double time_start = MPI_Wtime();

	if (g_rank == 0)
		static_master(argv[1], argv[2]);
	else
		slave();

	if (g_rank == 0)
		printf("time :%lg\n", MPI_Wtime() - time_start);

	MPI_Finalize();
	return 0;
}
