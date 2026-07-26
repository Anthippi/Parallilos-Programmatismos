#include <stdio.h>
#include <math.h>
#include <sys/time.h> 

#define N 100000000  

double f(double x) {
    return x*x; 
}

int main() {
    double a = 0.0, b = 10.0;
    double h = (b - a) / N;
    double sum = 0.0;

    struct timeval start, end;
    gettimeofday(&start, NULL);  // start

    for (int i = 0; i < N; i++) {
        double x1 = a + i*h;
        double x2 = a + (i+1)*h;
        sum += (f(x1) + f(x2)) * h / 2.0;
    }

    gettimeofday(&end, NULL);  // end

    double elapsed = (end.tv_sec - start.tv_sec) + 
                     (end.tv_usec - start.tv_usec) / 1e6;

    printf("Integral from %.2f to %.2f = %f\n", a, b, sum);
    printf("Execution time: %.6f seconds\n", elapsed);

    return 0;
}