#include <mpi.h>
#include <stdio.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define print_trace() 						\
do {								\
	printf("[%.3d]: print_trace: %s: (" __FILE__ ": %d)\n",	\
			get_mpi_rank(), __func__, __LINE__);	\
	fflush(stdout);						\
} while (0)

#define print_dbg(fmt, ...) 					\
do {								\
	printf("[%.3d]: ", get_mpi_rank());			\
	printf(fmt, ##__VA_ARGS__);				\
	fflush(stdout);						\
} while (0)

int get_mpi_rank()
{
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	return rank;
}

int get_mpi_size()
{
	int size;
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	return size;
}

void send_int(int dest, int tag, int *msg)
{
	MPI_Send(msg, 1, MPI_INT, dest, tag, MPI_COMM_WORLD);
}

void recv_int(int src, int tag, int *msg, MPI_Status *status)
{
	MPI_Recv(msg, 1, MPI_INT, src, tag, MPI_COMM_WORLD, status);
}

FILE *tty_file;

void ttysync_root()
{
	char buf[32] = { };
	ttyname_r(fileno(stdout), buf, sizeof(buf));

	int size = get_mpi_size();
	for (int i = 1; i < size; ++i)
		MPI_Send(buf, sizeof(buf), MPI_CHAR, i, 0, MPI_COMM_WORLD);

	tty_file = fopen(buf, "w");
	assert(tty_file);
}

void ttysync_node()
{
	char buf[32] = { };
	MPI_Recv(buf, sizeof(buf), MPI_CHAR, 0, 0, MPI_COMM_WORLD, NULL);
	tty_file = fopen(buf, "w");
	assert(tty_file);
}

void ttysync()
{
	if (get_mpi_rank() == 0)
		ttysync_root();
	else
		ttysync_node();
}

void print_hello()
{
	fprintf(tty_file, "[%.3d]\n", get_mpi_rank());
}

void test1()
{
	int const rank = get_mpi_rank();
	int const size = get_mpi_size();
	int dummy_msg = 0;

	if (rank != 0)
		recv_int(rank - 1, 0, &dummy_msg, NULL);

	print_hello();

	if (rank != size - 1)
		send_int(rank + 1, 0, &dummy_msg);
}

void test2()
{
	int const rank = get_mpi_rank();
	int const size = get_mpi_size();
	int dummy_msg = 0;

	if (rank != 0) {
		recv_int(0, 0, &dummy_msg, NULL);
		print_hello();
		send_int(0, 0, &dummy_msg);
	} else {
		print_hello();
		for (int i = 1; i < size; ++i) {
			send_int(i, 0, &dummy_msg);
			recv_int(i, 0, &dummy_msg, NULL);
		}
	}
}

int main(int argc, char **argv)
{
	MPI_Init(&argc, &argv);
	ttysync();

#ifdef 0
	test1();
#else
	test2()
#endif
	MPI_Finalize();
	return 0;
}
