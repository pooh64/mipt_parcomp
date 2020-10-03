#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include <mpi.h>

int g_size, g_rank;

struct eval_task {
	size_t beg;
	size_t sz;
};

void eval_split(struct eval_task *tasks, size_t n_tasks, struct eval_task full)
{
	while (n_tasks) {
		size_t sz = full.sz / n_tasks;
		tasks->beg = full.beg;
		tasks->sz = sz;

		full.beg += sz;
		full.sz -= sz;
		n_tasks--;
		tasks++;
	}
}

static inline float eval_term(size_t i)
{
	return (6 / (M_PI * M_PI)) / (i * i);
}

float eval_sum(struct eval_task *task)
{
	float accum = 0;
	if (task->sz < 1)
		return 0;
	size_t i = task->beg + task->sz - 1;
	do {
		accum += eval_term(i);
	} while (i-- != task->beg);
	return accum;
}

float eval_sum_collect(float *buf, size_t sz)
{
	float accum = 0;
	for (buf = buf + sz - 1; sz != 0; --sz, --buf)
		accum += *buf;
	return accum;
}

int main(int argc, char **argv)
{
	MPI_Init(&argc, &argv);

	struct eval_task task, *task_buf;
	float sum, *sum_buf;
	double task_time;
	MPI_Comm_size(MPI_COMM_WORLD, &g_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &g_rank);

	if (argc != 2)
		return 1;
	task.beg = 1;
	task.sz = atol(argv[1]);
	if (task.sz < 1)
		return 1;

	MPI_Barrier(MPI_COMM_WORLD);

	if (g_rank == 0) {
		task_time = MPI_Wtime();
		task_buf = malloc(sizeof(*task_buf) * g_size);
		sum_buf = malloc(sizeof(*sum_buf) * g_size);
		eval_split(task_buf, g_size, task);
	}

	MPI_Scatter(task_buf, sizeof(*task_buf), MPI_CHAR,
			&task, sizeof(task), MPI_CHAR, 0, MPI_COMM_WORLD);
	sum = eval_sum(&task);
	MPI_Gather(&sum, sizeof(sum), MPI_CHAR,
			sum_buf, sizeof(*sum_buf), MPI_CHAR, 0, MPI_COMM_WORLD);

	if (g_rank == 0) {
		sum = eval_sum_collect(sum_buf, g_size);
		task_time = MPI_Wtime() - task_time;
		printf("sum = %.6e, time = %.3e\n", sum, task_time);
	}

	MPI_Finalize();
	return 0;
}
