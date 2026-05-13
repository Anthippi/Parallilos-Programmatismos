#include <stdio.h>
#include <math.h>
#include <sys/time.h>  
#include <omp.h>       

#define N 100000 
#define THRESHOLD 1000 
#define CHUNK_SIZE 1000 

double f(double x) {
    double temp = x;
    int weight = (int)(fabs(sin(x * 123.45)) * 600); 
    
    for (int j = 0; j < weight; j++) {
        temp = sqrt(temp * temp + 0.00000001); 
    }
    return temp * temp;
}

// synartisi gia divide and conquer
double task_recursive(double start_a, double h, int steps) { 
    // to kommati einai arketa mikro to thread to ypologizei seiriaka.
    if (steps <= THRESHOLD) {
        double local_sum = 0.0;
        for (int i = 0; i < steps; i++) {
            double x1 = start_a + i*h;
            double x2 = start_a + (i+1)*h;
            local_sum += (f(x1) + f(x2)) * h / 2.0;
        }
        return local_sum;
    } 
    // spao to provlima sti mesi
    else {
        int mid = steps / 2;
        double sum1 = 0.0, sum2 = 0.0;

        // dimiourgo ena asygxrono task gia to aristero miso
        #pragma omp task shared(sum1)
        sum1 = task_recursive(start_a, h, mid);
        // kai gia to deksi miso 
        #pragma omp task shared(sum2)
        sum2 = task_recursive(start_a + mid * h, h, steps - mid);

        // xwris auto tha epistrepsoume lathos apotelesma
        #pragma omp taskwait
        return sum1 + sum2;
    }
}

int main() {
    double a = 0.0, b = 10.0;
    double h = (b - a) / N;
    
    int thread_counts[] = {1, 2, 3, 4, 8, 12};
    int num_configs = sizeof(thread_counts) / sizeof(thread_counts[0]);

    for (int t = 0; t < num_configs; t++) {
        int threads = thread_counts[t];
        omp_set_num_threads(threads);
        
        double total_elapsed_linear = 0.0; 
        double sum_linear = 0.0;

        for (int run = 0; run < 20; run++) {
            sum_linear = 0.0; 
            struct timeval start, end;
            gettimeofday(&start, NULL);  

            // dimiourgo ti deksameni twn threads
            #pragma omp parallel 
            {
                // mono ena thread prepei na treksei ti loupa kai na gennisei ta tasks  
                // kai ta ypoloipa threads perimenoun sti deksameni kai pairnoun ta tasks molis dimiourgithoun

                #pragma omp single
                {
                    for (int i = 0; i < N; i += CHUNK_SIZE) {
                        #pragma omp task
                        {
                            double local_sum = 0.0;
                            int end_i = (i + CHUNK_SIZE > N) ? N : i + CHUNK_SIZE;
                            for (int j = i; j < end_i; j++) {
                                double x1 = a + j*h;
                                double x2 = a + (j+1)*h;
                                local_sum += (f(x1) + f(x2)) * h / 2.0;
                            }
                            // asfalis athroisi tou merikou apotelesmatos sto global sum
                            #pragma omp atomic
                            sum_linear += local_sum;
                        }
                    }
                }
            }

            gettimeofday(&end, NULL);  
            total_elapsed_linear += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
        }

        double total_elapsed_recursive = 0.0; 
        double sum_recursive = 0.0;

        for (int run = 0; run < 20; run++) {
            sum_recursive = 0.0; 
            struct timeval start, end;
            gettimeofday(&start, NULL);  

            #pragma omp parallel 
            {
                // mono ena thread kalei tin anadromiki synartisi gia prwti fora
                // ayti i synartisi meta tha arxisei na gennaei apo moni tis nea tasks 
                // pou tha ta paroun ta ypoloipa idle threads
                #pragma omp single
                {
                    sum_recursive = task_recursive(a, h, N);
                }
            }

            gettimeofday(&end, NULL);  
            total_elapsed_recursive += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
        }

        char file_linear[60], file_recursive[60];
        sprintf(file_linear, "results_tasks_linear_%d_threads.txt", threads);
        sprintf(file_recursive, "results_tasks_recursive_%d_threads.txt", threads);
        
        FILE *fp_l = fopen(file_linear, "w");
        if (fp_l) {
            fprintf(fp_l, "Number of Threads: %d\n", threads);
            fprintf(fp_l, "Integral from %.2f to %.2f = %f\n", a, b, sum_linear);
            fprintf(fp_l, "Average execution time (20 runs): %.6f seconds\n", total_elapsed_linear / 20.0);
            fclose(fp_l);
        }

        FILE *fp_r = fopen(file_recursive, "w");
        if (fp_r) {
            fprintf(fp_r, "Number of Threads: %d\n", threads);
            fprintf(fp_r, "Integral from %.2f to %.2f = %f\n", a, b, sum_recursive);
            fprintf(fp_r, "Average execution time (20 runs): %.6f seconds\n", total_elapsed_recursive / 20.0);
            fclose(fp_r);
        }
    }
    return 0;
}