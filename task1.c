#include <mpi.h>
#include <stdio.h>

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

#define print_trace() 						\
do {								\
	printf("[%.3d]: print_trace: %s: (" __FILE__ ": %d)\n",	\
			get_mpi_rank(), __func__, __LINE__);	\
	fflush(stdout);						\
} while (0)

void test1()
{
	int const rank = get_mpi_rank();
	int const size = get_mpi_size();
	int dummy_msg = 0;

	if (rank != 0)
		recv_int(rank - 1, 0, &dummy_msg, NULL);

	print_trace();

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
		print_trace();
		send_int(0, 0, &dummy_msg);
	} else {
		print_trace();
		for (int i = 0; i < size; ++i) {
			send_int(i, 0, &dummy_msg);
			recv_int(i, 0, &dummy_msg, NULL);
		}
	}
}

int main(int argc, char **argv)
{
	MPI_Init(&argc, &argv);
	test1();
	//test2();
	MPI_Finalize();
	return 0;
}
