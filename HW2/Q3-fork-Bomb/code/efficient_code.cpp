#include <iostream>
#include <cmath>
#include <numeric>
#include <omp.h> // Include OpenMP header

using namespace std;

double slowFunction() {
    double result = 0.0;
    long N = 500000000;

    // Use OpenMP to parallelize the loop and reduce the final results
    // The 'reduction(+:result)' clause safely combines the intermediate results
    // calculated by each thread into the final 'result' variable.
    #pragma omp parallel for reduction(+:result)
    for (long i = 0; i < N; i++) {
        // Use the double-precision math functions from <cmath>
        result += sin((double)i) * cos((double)i / 2.0);
    }

    return result;
}

int main() {
    // Optional: Measure execution time for comparison
    // double start_time = omp_get_wtime();
    
    cout << "Result: " << slowFunction() << endl;
    
    // double end_time = omp_get_wtime();
    // cout << "Execution Time: " << (end_time - start_time) << " seconds" << endl;
    
    return 0;
}
