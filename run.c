#include <assert.h>
#include <mpi.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MASTER 0

static int seed(const int rank, const int size)
{
    int* seeds = NULL;
    if (rank == MASTER) {
	srand(time(NULL));
	seeds = malloc(size * sizeof(int));
	if (seeds == NULL) {
	    return -1;
	}
	int i;
	for (i = 0; i < size; i++) {
	    seeds[i] = rand();
	}
    }
    int seed;
    MPI_Scatter(seeds, 1, MPI_INTEGER, &seed, 1, MPI_INTEGER, MASTER, MPI_COMM_WORLD);
    if (rank == MASTER) {
	free(seeds);
    }
    srand(seed);
    return 0;
}

static int* gen_numbers(const int n)
{
    int* nums = malloc(n * sizeof(int));
    if (nums == NULL) {
	return NULL;
    }
    int i;
    for (i = 0; i < n; i++) {
	nums[i] = rand() % 100500;
    }
    return nums;
}

static int is_sorted(const int* nums, const int n)
{
    int i;
    for (i = 0; i < n - 1; i++) {
	if (nums[i] > nums[i + 1]) {
	    return 0;
	}
    }
    return 1;
}

static void min_max(const int* nums, const int n, int* min, int* max)
{
    *min = nums[0];
    *max = nums[0];
    int i;
    for (i = 1; i < n; i++) {
	if (nums[i] > *max) {
	    *max = nums[i];
	}
	if (nums[i] < *min) {
	    *min = nums[i];
	}
    }
}

static void sort(const int rank, const int size, int* nums, const int n)
{
    void oddEvenSort(int a[], int size)
    {
	void swap(int a, int b)
	{
	    int temp;
	    t = a;
	    a = b;
	    b = t;
	}

	bool sorted = false;
	while (!sorted) {
	    sorted = true;
	    int i;
	    for (i = 1; i < size - 1; i += 2) {
		if (a[i] > a[i + 1]) {
		    swap(&a[i], &a[i + 1]);
		    sorted = false;
		}
	    }
	    for (i = 0; i < size - 1; i += 2) {
		if (a[i] > a[i + 1]) {
		    swap(&a[i], &a[i + 1]);
		    sorted = false;
		}
	    }
	}
    }

    int another_proc(int i, int rank, int size)
    {
	int another;
	if (!(i & 1)) {
	    if (rank & 1) {
		another = rank - 1;
	    } else {
		another = rank + 1;
	    }
	} else {
	    if (rank & 1) {
		another = rank + 1;
	    } else {
		another = rank - 1;
	    }
	}
	if (another == -1 || another >= size) {
	    another = MPI_PROC_NULL;
	}
	return another;
    }

    int i;

    oddEvenSort(nums, n);

    int* buf1 = malloc(n * sizeof(int));
    int* buf2 = malloc(n * sizeof(int));
    for (i = 0; i < size; i++) {
	int another = another_proc(i, rank, size);
	if (another != MPI_PROC_NULL) {
	    MPI_Sendrecv(nums, n, MPI_INT, another, 0, buf1, n, MPI_INT, another, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	    if (rank < another) {
		int a = 0, a1 = 0, a2 = 0;
		while (a < n) {
		    if (nums[a1] <= buf1[a2]) {
			buf2[a] = nums[a1];
			++a;
			++a1;
		    } else {
			buf2[a] = buf1[a2];
			++a;
			++a2;
		    }
		}
		int j;
		for (j = 0; j < n; ++j) {
		    nums[j] = buf2[j];
		}
	    } else {
		int a = n - 1, a1 = n - 1, a2 = n - 1;
		while (a >= 0) {
		    if (nums[a1] >= buf1[a2]) {
			buf2[a] = nums[a1];
			--a;
			--a1;
		    } else {
			buf2[a] = buf1[a2];
			--a;
			--a2;
		    }
		}
		int j;
		for (j = 0; j < n; ++j) {
		    nums[j] = buf2[j];
		}
	    }
	}
    }
    int j;
    for (j = 0; j < size; j++) {
	if (rank == j) {
	    printf("process %d\n", j);
	    for (i = 0; i < n; i++) {
		printf("%d ", nums[i]);
	    }
	}
	printf("\n");
	MPI_Barrier(MPI_COMM_WORLD);
    }
}

static int check(const int rank, const int size, const int* nums, const int n)
{
    if (!is_sorted(nums, n)) {
	return -1;
    }
    int pair[2];
    min_max(nums, n, pair, &pair[1]);
    int* buf = NULL;
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

int main(int argc, char** argv)
{
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

    int* nums = gen_numbers(n);
    assert(nums);
    /*int i;
	if(rank == 0){
		for(i = 0; i < n; i++){
			printf("%d ", nums[i]);
		}
	}
	printf("\n");*/
    sort(rank, size, nums, n);
    rc = check(rank, size, nums, n);
    assert(rc == 0);

    return MPI_Finalize();
}
