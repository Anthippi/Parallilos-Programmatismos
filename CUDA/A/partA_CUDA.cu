#include <stdio.h>
#include <math.h>
#include <cuda.h>
#include <cuda_runtime.h>


__device__ double f_regular(double x) {
    return x * x; 
}

__device__ double f_irregular(double x) {
    double temp = x;
    int weight = (int)(fabs(sin(x * 123.45)) * 600);
    for (int j = 0; j < weight; j++) {
        temp = sqrt(temp * temp + 0.000000001);
    }
    return temp * temp;
}

// kathorisi poia f tha treksi analoga to flag
__device__ double get_f(double x, bool irregular) {
    return irregular ? f_irregular(x) : f_regular(x);
}

// Naive
// kathe nima grafete katefthia sto global memory
__global__ void kernel_naive(double a, double h, int N, double *d_sum, bool irregular) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < N) {
        double x1 = a + i*h;
        double x2 = a + (i+1)*h;
        atomicAdd(d_sum, (get_f(x1, irregular) + get_f(x2, irregular)) * h / 2.0);
    }
}

// grid-stride me global memory 
// ta nomata kanoun polles doulies gia na kalipsoun ta N
__global__ void kernel_global(double a, double h, int N, double *d_sum, bool irregular) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    while (i < N) {
        double x1 = a + i*h;
        double x2 = a + (i+1)*h;
        atomicAdd(d_sum, (get_f(x1, irregular) + get_f(x2, irregular)) * h / 2.0);
        i += blockDim.x * gridDim.x;
    }
}

// grid-stride me registers 
// kathe nima xrisimopoiei dikotou local_sum 
__global__ void kernel_registers(double a, double h, int N, double *d_sum, bool irregular) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    double local_sum = 0.0;
    while (i < N) {
        double x1 = a + i*h;
        double x2 = a + (i+1)*h;
        local_sum += (get_f(x1, irregular) + get_f(x2, irregular)) * h / 2.0;
        i += blockDim.x * gridDim.x;
    }
    atomicAdd(d_sum, local_sum);
}

// optimal me shared memory
// ta nimata kathe block exoun mia kini metagliti opou ekei athizoun ola tis times tous
// kai sto telos ena nima epistrefei to apotelesma
__global__ void kernel_shared(double a, double h, int N, double *d_sum, bool irregular) {
    extern __shared__ double sdata[]; // i kini timi mpainei sto sdata
    int tid = threadIdx.x;
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    
    double local_sum = 0.0;
    while (i < N) {
        double x1 = a + i*h;
        double x2 = a + (i+1)*h;
        local_sum += (get_f(x1, irregular) + get_f(x2, irregular)) * h / 2.0;
        i += blockDim.x * gridDim.x;
    }
    sdata[tid] = local_sum;
    __syncthreads(); // perimenoun ta nimata na teliosoun ola

    // athisma apotelesmaton
    for (int s = blockDim.x / 2; s > 0; s >>= 1) {
        if (tid < s) sdata[tid] += sdata[tid + s];
        __syncthreads();
    }

    // to arxiko nima epistrefei to apotelesma
    if (tid == 0) atomicAdd(d_sum, sdata[0]);
}

int main() {
    double a = 0.0, b = 10.0;
    int thread_counts[] = {32, 64, 128, 256, 512};
    const char* kernel_names[] = {"naive", "global", "registers", "shared"};
    int N_values[] = {1000, 10000, 100000, 1000000, 100000000};
    bool irregular_modes[] = {false, true}; 

    for (bool irregular : irregular_modes) {
        for (int threads : thread_counts) {
            for (int k = 0; k < 4; k++) {
                for (int N : N_values) {

                    // apofegvo megala N sto Irregular gia na min pathi TDR
                    if (irregular && N > 1000000) continue;
                    
                    double h = (b - a) / N;
                    double sum = 0.0;
                    double *d_sum;
                    
                    // desmevo monimi apo tin karta grafikon
                    cudaMalloc((void **)&d_sum, sizeof(double));
                    
                    // upologismos ton blocks tou grid kai shared mnimis
                    int blocks = (k == 0) ? ((N + threads - 1) / threads) : 1024;
                    int shared_mem = threads * sizeof(double);
                    
                    double total_elapsed = 0.0;

                    for (int run = 0; run < 20; run++) {
                        sum = 0.0;
                        cudaMemcpy(d_sum, &sum, sizeof(double), cudaMemcpyHostToDevice); // arxikopoiisi d_sum
                        
                        cudaEvent_t start, end;
                        cudaEventCreate(&start); cudaEventCreate(&end);
                        cudaEventRecord(start);

                        // klisi tou antisixou kernel
                        if (k == 0) kernel_naive<<<blocks, threads>>>(a, h, N, d_sum, irregular);
                        else if (k == 1) kernel_global<<<blocks, threads>>>(a, h, N, d_sum, irregular);
                        else if (k == 2) kernel_registers<<<blocks, threads>>>(a, h, N, d_sum, irregular);
                        else if (k == 3) kernel_shared<<<blocks, threads, shared_mem>>>(a, h, N, d_sum, irregular);

                        // telos metriseon
                        cudaEventRecord(end);
                        cudaEventSynchronize(end); 
                        
                        float ms;
                        cudaEventElapsedTime(&ms, start, end);
                        total_elapsed += (ms / 1000.0);
                        
                        cudaEventDestroy(start); cudaEventDestroy(end);
                    }
   
                    double avg_elapsed = total_elapsed / 20.0;
                    
                    // metafora tou telikou apotelesmatos apo tin gpu stin cpu
                    cudaMemcpy(&sum, d_sum, sizeof(double), cudaMemcpyDeviceToHost);
                    cudaFree(d_sum);

                    printf("Mode: %s | Threads: %3d | Kernel: %-9s | N: %9d | Avg Time: %.6f sec\n", 
                        irregular ? "Irregular" : "Regular", threads, kernel_names[k], N, avg_elapsed);

                    char filename[100];
                    sprintf(filename, "results_%s_%s_threads%d_N_%d.txt", irregular ? "irreg" : "reg", kernel_names[k], threads, N);
                    
                    FILE *fp = fopen(filename, "w");
                    if (fp != NULL) {

                        fprintf(fp, "Integral from %.2f to %.2f = %f\n", a, b, sum);
                        fprintf(fp, "Average execution time (20 runs): %.6f seconds\n", avg_elapsed);
                        fclose(fp);
                    } else {
                        printf("Error: Could not create file %s\n", filename);
                    }
                }
            }
        }
    }
    return 0;
}