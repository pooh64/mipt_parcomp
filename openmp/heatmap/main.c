#include <omp.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>

long get_int(char *str)
{
	char *eptr;
	errno = 0;
	long val = strtol(str, &eptr, 0);
	if (*eptr || errno || val < INT_MIN || val > INT_MAX) {
		fprintf(stderr, "Wrong num format\n");
		exit(1);
	}
	return val;
}

int create_threads(int i)
{
	int res = 0;
	#pragma omp parallel num_threads(i)
	{
		res++;
	}

	return res;
}

void work(int x, int y)
{
	omp_set_nested(1);
	int res = 0;

	#pragma omp parallel num_threads(x)
	{
		for (int i = 0; i < 128; ++i)
			res += create_threads(y);
	}
}

int main(int argc, char **argv)
{
	if (argc != 3) {
		fprintf(stderr, "Wrong argv\n");
		exit(1);
	}

	int x = get_int(argv[1]);
	int y = get_int(argv[2]);

	double tmes = omp_get_wtime();
	work(x, y);
	tmes = omp_get_wtime() - tmes;

	printf("%10.5f", (float) tmes);

	return 0;
}
