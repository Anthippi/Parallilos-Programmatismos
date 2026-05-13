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

            // ola ta threads ksekinane mazi edw.
            #pragma omp parallel 
            {
                // kathe thread dhmiourgei to diko tou 'local_sum' stin mnimi tou
                // to kathe thread athroizei topika to diko tou kommati apo tis epanalipseis 
                // etsi apofeygo to false sharing
                // kai glutono ta sinexomena locks stin kentriki metavliti 'sum'
                double local_sum = 0.0; 

                // to pragma omp for moirazei aytomata kai dikaia tis N epanalipseis 
                // tis for-loop sta diathesima threads xrisimopoiei static scheduling by default
                #pragma omp for 
                for (int i = 0; i < N; i++) {
                    double x1 = a + i*h;
                    double x2 = a + (i+1)*h;
                    local_sum += (f(x1) + f(x2)) * h / 2.0;
                }

                // to kathe thread erxetai edw sto telos gia na dwsei to teliko tou noumero
                // evala atomic gia ti prosthesi tou merikou apotelesmatos sto global 'sum'
                // me asfaleia ena thread ti fora
                // epeidi afto einai ekso apo tin for to lock ginetai mono mia fora ana thread
                // etsi exei min lock contention
                #pragma omp atomic
                sum += local_sum;
            }

            gettimeofday(&end, NULL);  

            double elapsed = (end.tv_sec - start.tv_sec) + 
                             (end.tv_usec - start.tv_usec) / 1e6;
                             
            total_elapsed += elapsed; 
        }

        double avg_elapsed = total_elapsed / 20.0; 
        char filename[50];
        sprintf(filename, "results_%d_threads.txt", threads);
        
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