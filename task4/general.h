#ifndef _GENERAL_H_
#define _GENERAL_H_

#include <stddef.h>

#define MAX_ROUTINE_INT (1000L * 1000L * 1000L - 1)
#define TAG_TASK  1
#define TAG_START 2
#define TAG_EXIT  3

struct routine_in {
	int arr[2];
};
struct routine_out {
	int arr[2];
	char cf;
};
struct task_in {
	size_t task_id;
	struct routine_in data;
};
struct task_out {
	size_t task_id;
	struct routine_out data;
};

extern int g_size;
extern int g_rank;

void routine(struct routine_in *in, struct routine_out *out);
void read_inpfile(char *path, struct routine_in **in, size_t *in_s);
void write_outfile(char *path, struct routine_out *out, size_t out_s);
void master_start(int task_sz);
void master_shutdown();
void slave();

#endif
