#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <pthread.h>

#define N 100000 
#define NUM_RUNS 20

double *results;
int num_threads;


// oso megalitero x tosos pio megali timi epistrefei ara argei perissotero
double f(double x) {
    double temp = x;
    for (int j = 0; j < (int)(x * 100); j++) {
        temp = sqrt(temp * temp + 0.00000001); 
    }
    return temp * temp;
}

// sikilikos diamiramso nimatos
void* cyclic_worker(void* arg) {
    int tid = *(int*)arg;
    double a = 0.0, b = 10.0;
    double h = (b - a) / N;
    double local_sum = 0.0;

    // kiklikos diamirasmos
    for (int i = tid; i < N; i += num_threads) {
        double x1 = a + i * h;
        double x2 = a + (i + 1) * h;
        local_sum += (f(x1) + f(x2)) * h / 2.0;
    }

    results[tid] = local_sum;
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <num_threads>\n", argv[0]);
        return 1;
    }
    
    num_threads = atoi(argv[1]);
    results = malloc(num_threads * sizeof(double));

    double total_time = 0.0;
    double final_sum = 0.0;
    double a = 0.0, b = 10.0;

    printf("Part B\n");

    for (int run = 0; run < NUM_RUNS; run++) {
        pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
        int *tids = malloc(num_threads * sizeof(int));
        struct timeval start, end;

        gettimeofday(&start, NULL);

        // dimiourgia nimatos
        for (int i = 0; i < num_threads; i++) {
            tids[i] = i;
            pthread_create(&threads[i], NULL, cyclic_worker, &tids[i]);
        }

        final_sum = 0.0;
        // join kai apothikefti topikon apotelesmaton
        for (int i = 0; i < num_threads; i++) {
            pthread_join(threads[i], NULL);
            final_sum += results[i];
        }

        gettimeofday(&end, NULL);
        double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
        total_time += elapsed;

        free(threads);
        free(tids);
    }

    double avg_time = total_time / NUM_RUNS;
    
    printf("------------------------------------------------------------\n");
    printf("Final Integral from %.2f to %.2f = %f\n", a, b, final_sum);
    printf("Average Execution Time (%d runs): %.6f seconds\n", NUM_RUNS, avg_time);

    free(results);
    return 0;
}