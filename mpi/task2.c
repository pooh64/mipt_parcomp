#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include <mpi.h>

/*****************************************************************************/
/* meausure accumulator */

typedef struct msr_accum {
	long double sum_x;
	long double sum_x2;
	size_t n_msr;
} msr_accum_t;

static inline
void msr_accum_clear(msr_accum_t *c)
{
	c->sum_x = 0;
	c->sum_x2 = 0;
	c->n_msr = 0;
}

static inline
void msr_accum_add(msr_accum_t *c, double val)
{
	c->sum_x  += val;
	c->sum_x2 += (val * val);
	c->n_msr++;
}

static inline
double msr_accum_avg(msr_accum_t *c)
{
	return c->sum_x / c->n_msr;
}

static inline
double msr_accum_sd(msr_accum_t *c)
{
	long double sum = c->sum_x2 - c->sum_x * c->sum_x / c->n_msr;
	return sqrt(sum / c->n_msr);
}

static inline
double msr_accum_rmse(msr_accum_t *c)
{
	long double sum = c->sum_x2 - c->sum_x * c->sum_x / c->n_msr;
	return sqrt(sum / (c->n_msr * (c->n_msr - 1)));
}

/*****************************************************************************/

int *__routine_buf1, *__routine_buf2;

void measure_while_prec(msr_accum_t *accum, double prec,
	double (*routine)(void))
{
	int size;
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	__routine_buf1 = malloc(sizeof(int) * size);
	__routine_buf2 = malloc(sizeof(int) * size);
	assert(__routine_buf1);
	assert(__routine_buf2);

	msr_accum_clear(accum);

	for (int i = 0; i < 10; ++i)
		msr_accum_add(accum, routine());

	for (int i = 0; i < 10000 && msr_accum_rmse(accum) > prec; ++i)
		msr_accum_add(accum, routine());

	free(__routine_buf1);
	free(__routine_buf2);
}

#define MEASURE_ROUTINE(name, code)		\
double __measure_routine_##name ()		\
{						\
	MPI_Barrier(MPI_COMM_WORLD);		\
	double rv = MPI_Wtime();		\
	code;					\
	MPI_Barrier(MPI_COMM_WORLD);		\
	rv = MPI_Wtime() - rv;			\
	return rv;				\
}

MEASURE_ROUTINE(empty, ;)

MEASURE_ROUTINE(MPI_Bcast,
	MPI_Bcast(__routine_buf1, 1, MPI_INT, 0, MPI_COMM_WORLD))

MEASURE_ROUTINE(MPI_Reduce,
	MPI_Reduce(__routine_buf1, __routine_buf2, 1, MPI_INT, MPI_SUM,
		0, MPI_COMM_WORLD))

MEASURE_ROUTINE(MPI_Scatter,
	MPI_Scatter(__routine_buf1, 1 , MPI_INT, __routine_buf2, 1, MPI_INT,
		0, MPI_COMM_WORLD))

MEASURE_ROUTINE(MPI_Gather,
	MPI_Gather(__routine_buf1, 1, MPI_INT, __routine_buf2, 1, MPI_INT, 0,
		MPI_COMM_WORLD))

#undef MEASURE_ROUTINE

int main(int argc, char **argv)
{
	MPI_Init(&argc, &argv);

	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	msr_accum_t accum;
	double prec = MPI_Wtick();

#define MEASURE_ROUTINE(name) do {					\
	msr_accum_clear(&accum);					\
	measure_while_prec(&accum, prec, __measure_routine_##name);	\
	if (rank == 0) {						\
		printf("--------routine: " #name			\
			"\navg = %.4e sd = %.4e rmse = %.4e\n",		\
			msr_accum_avg(&accum), msr_accum_sd(&accum),	\
			msr_accum_rmse(&accum));			\
	}} while (0)
	MEASURE_ROUTINE(empty);
	MEASURE_ROUTINE(MPI_Bcast);
	MEASURE_ROUTINE(MPI_Reduce);
	MEASURE_ROUTINE(MPI_Scatter);
	MEASURE_ROUTINE(MPI_Gather);
#undef MEASURE_ROUTINE

	MPI_Finalize();
	return 0;
}
