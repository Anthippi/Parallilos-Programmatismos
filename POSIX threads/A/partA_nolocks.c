#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>

#define N 100000000  
#define NUM_RUNS 20 

// global metavlites
double a = 0.0, b = 10.0;
double h;
int num_threads;

// gia tin apothikefti apotelesmatos kathe nimatos
double *thread_results;

double f(double x) {
    return x * x; 
}

// perasma orismaton sto nima
typedef struct {
    int thread_id;
} thread_data;

void* worker_nolocks(void* arg) {
    thread_data* data = (thread_data*)arg;
    int tid = data->thread_id;
    
    // statikos diamirasmos
    int chunk_size = N / num_threads;
    int start = tid * chunk_size;
    int end = (tid == num_threads - 1) ? N : (tid + 1) * chunk_size;

    double local_sum = 0.0;

    for (int i = start; i < end; i++) {
        double x1 = a + i * h;
        double x2 = a + (i + 1) * h;
        local_sum += (f(x1) + f(x2)) * h / 2.0;
    }

    // apothikefti tou topikou athrismatos stin thesi tou nimatos ston pinaka
    thread_results[tid] = local_sum;

    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <num_threads>\n", argv[0]);
        return 1;
    }
    num_threads = atoi(argv[1]);
    if (num_threads <= 0) num_threads = 1;

    h = (b - a) / N;

    // desmefsi minimis gia tin apothikefti ton topikon apotelesematon analoga ton nimaton
    thread_results = (double *)malloc(num_threads * sizeof(double));
    if (thread_results == NULL) {
        printf("Memory allocation failed!\n");
        return 1;
    }

    pthread_t threads[num_threads];
    thread_data t_data[num_threads];

    double total_time = 0.0;
    double final_integral = 0.0;

    printf("Part A WITHOUT Locks\n");
    printf("--------------------------------------------------\n");

    for (int run = 0; run < NUM_RUNS; run++) {
        
        // midenismo tou pinaka apotelematon kai tou telikou olokliromatos gia kathe ektelesi
        for (int i = 0; i < num_threads; i++) {
            thread_results[i] = 0.0;
        }
        final_integral = 0.0; 

        struct timeval start_time, end_time;
        gettimeofday(&start_time, NULL);

      // dimiourgia nimaton
        for (int i = 0; i < num_threads; i++) {
            t_data[i].thread_id = i;
            pthread_create(&threads[i], NULL, worker_nolocks, (void*)&t_data[i]);
        }

        // anamoni gia termatismo nimaton me join 
        for (int i = 0; i < num_threads; i++) {
            pthread_join(threads[i], NULL);
        }

        // to kirio nima athrisi ta merika apotelesmata adou oloklirithoun ola ta nimata
        for (int i = 0; i < num_threads; i++) {
            final_integral += thread_results[i];
        }

        gettimeofday(&end_time, NULL);

        double elapsed = (end_time.tv_sec - start_time.tv_sec) + 
                         (end_time.tv_usec - start_time.tv_usec) / 1e6;
        total_time += elapsed;
    }

    double avg_time = total_time / NUM_RUNS;

    printf("--------------------------------------------------\n");
    printf("Final Integral from %.2f to %.2f = %f\n", a, b, final_integral);
    printf("Average Execution Time (%d runs): %.6f seconds\n", NUM_RUNS, avg_time);

    // apodesmefsi tis mmimis
    free(thread_results);

    return 0;
}