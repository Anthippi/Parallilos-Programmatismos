#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <pthread.h>

#define N 100000
#define NUM_RUNS 20
#define CHUNK_SIZE 100  // 100 epanalipsis ti fora

double *results;
int num_threads;
int current_task = 0;
pthread_mutex_t queue_mutex;

// imitonoidis sinatisi gia tixaies afksimiosris 
double f(double x) {
    double temp = x;
    // diskolia apo 0 - 600 epanalipsis. metio fortio. 123.45 tixios arithmos.
    int weight = (int)(fabs(sin(x * 123.45)) * 600); 
    
    for (int j = 0; j < weight; j++) {
        temp = sqrt(temp * temp + 0.00000001); 
    }
    return temp * temp;
}

// dianismatikos diamirasmos
void* dynamic_worker(void* arg) {
    int tid = *(int*)arg;
    double a = 0.0, b = 10.0;
    double h = (b - a) / N;
    double local_sum = 0.0;

    while (1) {
        int my_start, my_end;

        // klidoma tis ouras gia na parei to nima to paketo 
        pthread_mutex_lock(&queue_mutex);
        my_start = current_task;
        current_task += CHUNK_SIZE; afksisi dikti gia to epomeno 
        pthread_mutex_unlock(&queue_mutex);

        // eksofos ninamos
        if (my_start >= N) {
            break;
        }

        // telionei to paketo xotis na kseperasi to N
        my_end = my_start + CHUNK_SIZE;
        if (my_end > N) {
            my_end = N;
        }

        // ektelesi toy paketou ergasion
        for (int i = my_start; i < my_end; i++) {
            double x1 = a + i * h;
            double x2 = a + (i + 1) * h;
            local_sum += (f(x1) + f(x2)) * h / 2.0;
        }
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
    pthread_mutex_init(&queue_mutex, NULL);

    double total_time = 0.0;
    double final_sum = 0.0;
    double a = 0.0, b = 10.0;

    printf("Part C\n");
    printf("------------------------------------------------------------\n");

    for (int run = 0; run < NUM_RUNS; run++) {
        pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
        int *tids = malloc(num_threads * sizeof(int));
        struct timeval start, end;

        // midenismos tis ouras ergasios prin apo kathe run
        current_task = 0; 

        gettimeofday(&start, NULL);

        for (int i = 0; i < num_threads; i++) {
            tids[i] = i;
            pthread_create(&threads[i], NULL, dynamic_worker, &tids[i]);
        }

        final_sum = 0.0;
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
    
    printf("Final Integral from %.2f to %.2f = %f\n", a, b, final_sum);
    printf("Average Execution Time (%d runs): %.6f seconds\n", NUM_RUNS, avg_time);

    free(results);
    pthread_mutex_destroy(&queue_mutex);
    return 0;
}