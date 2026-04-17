#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>

#define N 100000000  
#define NUM_RUNS 20 

double total_sum = 0.0;
pthread_mutex_t lock;

// global metavlites
double a = 0.0, b = 10.0;
double h;
int num_threads;

double f(double x) {
    return x * x; 
}

// perasma orismaton sto nima
typedef struct {
    int thread_id;
    int is_first_run; // flag gia na vro tin proti fora pou emfanizetai
} thread_data;

void* worker_locks(void* arg) {
    thread_data* data = (thread_data*)arg;
    int tid = data->thread_id;
    
    // statikos diamirasmos
    int chunk_size = N / num_threads;
    int start = tid * chunk_size;
    int end = (tid == num_threads - 1) ? N : (tid + 1) * chunk_size;

    double local_sum = 0.0;
    
    // proodeftiki enimerosi
    // kathe nima tha enimeroni tin kentriki metavliti 4 fores
    int update_interval = chunk_size / 4;
    if (update_interval == 0) update_interval = 1;

    for (int i = start; i < end; i++) {
        double x1 = a + i * h;
        double x2 = a + (i + 1) * h;
        local_sum += (f(x1) + f(x2)) * h / 2.0;

        // proodeftiki enimerosi me kleisoma se kathorismena diastimata
        if ((i - start + 1) % update_interval == 0) {
            pthread_mutex_lock(&lock);
            total_sum += local_sum; // sinathrisi tis topikis doulias sto sinolo
            local_sum = 0.0;        // midenismos tis topikis metavlitis gia to epomeno
            
            // entiksi tis proodou mono gia tin proti ektelesi
            if (data->is_first_run) {
                printf("Thread %d update current value %f\n", tid, total_sum);
            }
            pthread_mutex_unlock(&lock);
        }
    }

    // protheto oti exei meinei
    if (local_sum > 0.0) {
        pthread_mutex_lock(&lock);
        total_sum += local_sum;
        pthread_mutex_unlock(&lock);
    }

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

    pthread_t threads[num_threads];
    thread_data t_data[num_threads];

    pthread_mutex_init(&lock, NULL);

    double total_time = 0.0;
    double final_result = 0.0;

    printf("Part A WITH Locks\n");
    printf("--------------------------------------------------\n");

    for (int run = 0; run < NUM_RUNS; run++) {
        total_sum = 0.0; 

        struct timeval start_time, end_time;
        gettimeofday(&start_time, NULL);

        // dimiourgia nimaton
        for (int i = 0; i < num_threads; i++) {
            t_data[i].thread_id = i;
            t_data[i].is_first_run = (run == 0); // True mono sto 1ο run
            pthread_create(&threads[i], NULL, worker_locks, (void*)&t_data[i]);
        }

        // anamoni gia termatismo nimaton me join 
        for (int i = 0; i < num_threads; i++) {
            pthread_join(threads[i], NULL);
        }

        gettimeofday(&end_time, NULL);

        final_result = total_sum;

        double elapsed = (end_time.tv_sec - start_time.tv_sec) + 
                         (end_time.tv_usec - start_time.tv_usec) / 1e6;
        total_time += elapsed;
    }

    double avg_time = total_time / NUM_RUNS;

    printf("--------------------------------------------------\n");
    printf("Final Integral from %.2f to %.2f = %f\n", a, b, final_result);
    printf("Average Execution Time (%d runs): %.6f seconds\n", NUM_RUNS, avg_time);

    pthread_mutex_destroy(&lock);

    return 0;
}