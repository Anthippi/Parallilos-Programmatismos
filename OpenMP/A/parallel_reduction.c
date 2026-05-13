#include <stdio.h>
#include <math.h>
#include <sys/time.h>  
#include <omp.h>       

#define N 100000000  

double f(double x) {
    return x*x; 
}

int main() {
    double a = 0.0, b = 10.0;
    double h = (b - a) / N;
    
    int thread_counts[] = {1, 2, 3, 4, 8, 12};
    int num_configs = sizeof(thread_counts) / sizeof(thread_counts[0]);

    for (int t = 0; t < num_configs; t++) {
        int threads = thread_counts[t];
        
        omp_set_num_threads(threads);
        
        double total_elapsed = 0.0; 
        double sum = 0.0;

        for (int run = 0; run < 20; run++) {
            sum = 0.0; 

            struct timeval start, end;
            gettimeofday(&start, NULL); 

            // to pragma omp parallel for dimiourgei ta threads kai moirazei tis N epanalipseis
            // to OpenMP ftiaxnei automata ena topiko antigrafo tis 'sum' gia kathe thread
            // kai kathe thread athroizei ston diko tou xwro opote apofeygoume entelws ta race conditions
            // kai to false sharing sto telos ola ta topika 'sum' kai ta prosthetei stin arxiki global metavliti 'sum'.

            #pragma omp parallel for reduction(+:sum)
            for (int i = 0; i < N; i++) {
                double x1 = a + i*h;
                double x2 = a + (i+1)*h;
                sum += (f(x1) + f(x2)) * h / 2.0;
            }

            gettimeofday(&end, NULL);  

            double elapsed = (end.tv_sec - start.tv_sec) + 
                             (end.tv_usec - start.tv_usec) / 1e6;
                             
            total_elapsed += elapsed; 
        }

        double avg_elapsed = total_elapsed / 20.0; 

        char filename[50];
        sprintf(filename, "results_reduction_%d_threads.txt", threads);
        
        FILE *fp = fopen(filename, "w");
        if (fp != NULL) {
            fprintf(fp, "Number of Threads: %d\n", threads);
            fprintf(fp, "Integral from %.2f to %.2f = %f\n", a, b, sum);
            fprintf(fp, "Average execution time (20 runs): %.6f seconds\n", avg_elapsed);
            fclose(fp);
        } else {
            printf("Error: Could not create file %s\n", filename);
        }
    }
    return 0;
}