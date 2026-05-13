#include <stdio.h>
#include <math.h>
#include <sys/time.h>  
#include <omp.h>       

#define N 100000  

// synartisi gia na dhmourghsei anisokatanomi fortou
double f(double x) {
    double temp = x;
    int weight = (int)(fabs(sin(x * 123.45)) * 600); 
    
    for (int j = 0; j < weight; j++) {
        temp = sqrt(temp * temp + 0.00000001); 
    }
    return temp * temp;
}

int main() {
    double a = 0.0, b = 10.0;
    double h = (b - a) / N;
    
    int thread_counts[] = {1, 2, 3, 4, 8, 12};
    int num_threads_configs = sizeof(thread_counts) / sizeof(thread_counts[0]);

    const char* sched_names[] = {"static", "dynamic", "guided"};

    int chunks[] = {1, 10, 100, 1000, 100000, 100000000};
    int num_chunks = sizeof(chunks) / sizeof(chunks[0]);

    for (int t = 0; t < num_threads_configs; t++) {
        int threads = thread_counts[t];
        omp_set_num_threads(threads);
        
        for (int sched = 0; sched < 3; sched++) {
            
            for (int c = 0; c < num_chunks; c++) {
                int chunk = chunks[c];
                
                double total_elapsed = 0.0; 
                double sum = 0.0;

                printf("Running: %2d threads | %-7s | chunk %4d \n", threads, sched_names[sched], chunk);

                for (int run = 0; run < 20; run++) {
                    sum = 0.0; 

                    struct timeval start, end;
                    gettimeofday(&start, NULL);  

                    if (sched == 0) {
                        // o diamoirismos ginetai tyfla prin ksekinisei i ektelesi
                        #pragma omp parallel for reduction(+:sum) schedule(static, chunk)
                        for (int i = 0; i < N; i++) {
                            double x1 = a + i*h;
                            double x2 = a + (i+1)*h;
                            sum += (f(x1) + f(x2)) * h / 2.0;
                        }
                    } else if (sched == 1) {
                        // ta threads zhtane paketa epanalipsewn apo mia kentriki oura kata tin ektelesi 
                        #pragma omp parallel for reduction(+:sum) schedule(dynamic, chunk)
                        for (int i = 0; i < N; i++) {
                            double x1 = a + i*h;
                            double x2 = a + (i+1)*h;
                            sum += (f(x1) + f(x2)) * h / 2.0;
                        }
                    } else if (sched == 2) {
                        // to megethos tou paketou den einai stathero. 
                        #pragma omp parallel for reduction(+:sum) schedule(guided, chunk)
                        for (int i = 0; i < N; i++) {
                            double x1 = a + i*h;
                            double x2 = a + (i+1)*h;
                            sum += (f(x1) + f(x2)) * h / 2.0;
                        }
                    }

                    gettimeofday(&end, NULL);  

                    double elapsed = (end.tv_sec - start.tv_sec) + 
                                     (end.tv_usec - start.tv_usec) / 1e6;
                                     
                    total_elapsed += elapsed; 
                }

                double avg_elapsed = total_elapsed / 20.0; 

                char filename[100];
                sprintf(filename, "results_%d_threads_%s_chunk_%d.txt", threads, sched_names[sched], chunk);
                
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
        }
    }
    return 0;
}