#include <assert.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MASTER 0

static int seed(const int rank, const int size) {
  int *seeds = NULL;
  if (rank == MASTER) {
    srand(time(NULL));
    seeds = malloc(size * sizeof(int));
    if (seeds == NULL) {
      return -1;
    }
    for (int i = 0; i < size; i++) {
      seeds[i] = rand();
    }
  }
  int seed;
  MPI_Scatter(seeds, 1, MPI_INTEGER, &seed, 1, MPI_INTEGER, MASTER,
              MPI_COMM_WORLD);
  if (rank == MASTER) {
    free(seeds);
  }
  srand(seed);
  return 0;
}

static int *gen_numbers(const int n) {
  int *nums = malloc(n * sizeof(int));
  if (nums == NULL) {
    return NULL;
  }
  for (int i = 0; i < n; i++) {
    nums[i] = rand() % 100500;
  }
  return nums;
}

static int is_sorted(const int *nums, const int n) {
  for (int i = 0; i < n - 1; i++) {
    if (nums[i] > nums[i + 1]) {
      return 0;
    }
  }
  return 1;
}

static void min_max(const int *nums, const int n, int *min, int *max) {
  *min = nums[0];
  *max = nums[0];
  for (int i = 1; i < n; i++) {
    if (nums[i] > *max) {
      *max = nums[i];
    }
    if (nums[i] < *min) {
      *min = nums[i];
    }
  }
}

static int check(const int rank, const int size, const int *nums, const int n) {
  if (!is_sorted(nums, n)) {
    return -1;
  }
  int pair[2];
  min_max(nums, n, pair, &pair[1]);
  int *buf = NULL;
  if (rank == MASTER) {
    buf = malloc(2 * size * sizeof(int));
    if (buf == NULL) {
      return -2;
    }
  }
  MPI_Gather(pair, 2, MPI_INTEGER, buf, 2, MPI_INTEGER, MASTER, MPI_COMM_WORLD);
  if (rank == MASTER) {
    int rc = is_sorted(buf, 2 * size);
    free(buf);
    return rc == 0;
  }
  return 0;
}

static void merge(int *c, const int *a, const int *b, int size_a, int size_b) {
  int pa = 0, pb = 0;
  for (int i = 0; i < size_a + size_b; i++) {
    if ((pb == size_b) || (pa != size_a && a[pa] < b[pb])) {
      c[i] = a[pa++];
    } else {
      c[i] = b[pb++];
    }
  }
}

static int cmp(const void *a, const void *b) { return *(int *)a - *(int *)b; }

static void sort(const int rank, const int size, int *nums, const int n) {

  int *merge_tmp;
  merge_tmp = malloc(sizeof(int) * 2 * n);
  int *recv_tmp;
  recv_tmp = malloc(sizeof(int) * n);
  qsort(nums, n, sizeof(int), cmp);

  int not_sorted = 1;
  while (not_sorted) {
    if ((rank % 2 == 1) && (rank != size - 1)) {
      MPI_Send(nums, n, MPI_INTEGER, rank + 1, 0, MPI_COMM_WORLD);
      MPI_Recv(nums, n, MPI_INTEGER, rank + 1, 1, MPI_COMM_WORLD,
               MPI_STATUS_IGNORE);
    } else {
      if ((rank != MASTER) && (rank != size - 1)) {
        MPI_Recv(recv_tmp, n, MPI_INTEGER, rank - 1, 0, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);
        merge(merge_tmp, nums, recv_tmp, n, n);
        MPI_Send(merge_tmp, n, MPI_INTEGER, rank - 1, 1, MPI_COMM_WORLD);
        for (int i = 0; i < n; i++) {
          nums[i] = merge_tmp[n + i];
        }
      }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank % 2 == 0) {
      MPI_Send(nums, n, MPI_INTEGER, rank + 1, 0, MPI_COMM_WORLD);
      MPI_Recv(nums, n, MPI_INTEGER, rank + 1, 1, MPI_COMM_WORLD,
               MPI_STATUS_IGNORE);
    } else {
      MPI_Recv(recv_tmp, n, MPI_INTEGER, rank - 1, 0, MPI_COMM_WORLD,
               MPI_STATUS_IGNORE);
      merge(merge_tmp, nums, recv_tmp, n, n);
      MPI_Send(merge_tmp, n, MPI_INTEGER, rank - 1, 1, MPI_COMM_WORLD);
      for (int i = 0; i < n; i++) {
        nums[i] = merge_tmp[n + i];
      }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    not_sorted = check(rank, size, nums, n);
    MPI_Bcast(&not_sorted, 1, MPI_INTEGER, MASTER, MPI_COMM_WORLD);
  }
  free(recv_tmp);
  free(merge_tmp);
}

int main(int argc, char **argv) {

  float time1, time2;
  time1 = clock();
  MPI_Init(&argc, &argv);

  if (argc != 2) {
    return -1;
  }
  int n = atoi(argv[1]);
  assert(n > 0);
  int rank, size;

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  int rc = seed(rank, size);
  assert(rc == 0);

  int *nums = gen_numbers(n);
  assert(nums);

  sort(rank, size, nums, n);
  rc = check(rank, size, nums, n);
  assert(rc == 0);

  MPI_Finalize();
  time2 = clock();

  for (int i = 0; i < n; i++)
    printf("%d ", nums[i]);
  printf("Time:%f s\n", (time2 - time1) / CLOCKS_PER_SEC);
  return 0;
}