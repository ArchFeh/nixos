#include "payload.h"
#include <assert.h>
#include <omp.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int count_zeroes(const unsigned char *hash, const int len)
{
	int cnt = 0;
	for (int i = 0; i < len; i++) {
		cnt += hash[i] == 0;
	}
	return cnt;
}

static int run_omp(const char *str, const int target_count,
		   const int n_threads)
{
	omp_set_num_threads(n_threads);
	int ret = 0;
#pragma omp parallel reduction(+ \
			       : ret)
	{
		struct payload_t *p = payload_gen(str);
		assert(p);

		unsigned char hash[32];
#pragma omp for schedule(dynamic)
		for (long i = 0; i < MAX_MAGIC; i++) {
			payload_set_magic(p, i);
			payload_checksum(p, hash);
			int cnt = count_zeroes(hash, sizeof(hash));
			ret += cnt == target_count;
		}
		payload_free(p);
	}
	return ret;
}

struct send_t {
	pthread_mutex_t mutex;
	long value;
	long max;
};

void send_init(struct send_t *send_p, long max)
{
	pthread_mutex_init(&send_p->mutex, NULL);
	send_p->value = 0;
	send_p->max = max;
}

void send_destory(struct send_t *send_p)
{
	pthread_mutex_destroy(&send_p->mutex);
}

long send_fetch(struct send_t *send_p)
{
	pthread_mutex_lock(&send_p->mutex);

	long value = send_p->value;
	if (value < send_p->max) {
		send_p->value++;
	}
	else {
		value = -1;
	}

	pthread_mutex_unlock(&send_p->mutex);

	return value;
}

struct gather_t {
	pthread_mutex_t mutex;
	int value;
};

void gather_init(struct gather_t *gather_ptr)
{
	pthread_mutex_init(&gather_ptr->mutex, NULL);
	gather_ptr->value = 0;
}

void gather_destroy(struct gather_t *gather_ptr)
{
	pthread_mutex_destroy(&gather_ptr->mutex);
}

void gather_give(struct gather_t *gather_ptr, int value)
{
	pthread_mutex_lock(&gather_ptr->mutex);

	gather_ptr->value += value;

	pthread_mutex_unlock(&gather_ptr->mutex);
}

struct work_args_t {
	const char *str;
	int target_count;
	struct send_t *send_p;
	struct gather_t *gather_ptr;
};

void *work(void *args_ptr)
{
	const char *str = ((struct work_args_t *)args_ptr)->str;
	int target_count = ((struct work_args_t *)args_ptr)->target_count;
	struct send_t *send_p =
	    ((struct work_args_t *)args_ptr)->send_p;
	struct gather_t *gather_ptr = ((struct work_args_t *)args_ptr)->gather_ptr;

	struct payload_t *p = payload_gen(str);
	assert(p);

	int ret = 0;
	unsigned char hash[32];

	while (1) {
		long i = send_fetch(send_p);
		if (i < 0)
			break;

		payload_set_magic(p, i);
		payload_checksum(p, hash);
		int cnt = count_zeroes(hash, sizeof(hash));
		ret += cnt == target_count;
	}

	payload_free(p);
	gather_give(gather_ptr, ret);

	return NULL;
}

static int run_pthreads(const char *str, const int target_count,
			const int n_threads)
{
	struct send_t send;
	send_init(&send, MAX_MAGIC);

	struct gather_t gather;
	gather_init(&gather);

	struct work_args_t work_args = {str, target_count, &send, &gather};

	pthread_t *pthreads = malloc(sizeof(pthread_t) * n_threads);
	for (int i = 0; i < n_threads; ++i) {
		pthread_create(pthreads + i, NULL, work, &work_args);
	}

	for (int j = 0; j < n_threads; ++j) {
		pthread_join(pthreads[j], NULL);
	}
	free(pthreads);

	int ret = gather.value;

	send_destory(&send);
	gather_destroy(&gather);

	return ret;
}

typedef int (*cb)(const char *str, const int target_count, const int n_threads);

struct result_t {
	double elapsed;
	int cnt;
};

static struct result_t timer(const cb f, const char *str,
			     const int target_count, const int n_threads)
{
	struct result_t res;
	double start = omp_get_wtime();
	res.cnt = f(str, target_count, n_threads);
	res.elapsed = omp_get_wtime() - start;
	return res;
}

static int check(const char *str, const int target_count, const int n_threads)
{
	struct result_t r1 = timer(run_omp, str, target_count, n_threads);
	printf("OpenMP: cnt = %d, elapsed = %lfs\n", r1.cnt, r1.elapsed);
	struct result_t r2 = timer(run_pthreads, str, target_count, n_threads);
	printf("pthreads: cnt = %d, elapsed = %lfs\n", r2.cnt, r2.elapsed);
	if (r1.cnt != r2.cnt) {
		printf("Unexpected count: got %d, want %d\n", r2.cnt, r1.cnt);
		return -1;
	}
	if (0.9 * r2.elapsed > r1.elapsed) {
		printf("pthreads version is %lf times slower than OpenMP one\n",
		       r2.elapsed / r1.elapsed);
		return -2;
	}
	return 0;
}

int main(int argc, char **argv)
{
	char *data = argv[1];
	int target_count = atoi(argv[2]);
	int n_threads = atoi(argv[3]);
	return check(data, target_count, n_threads);
}
