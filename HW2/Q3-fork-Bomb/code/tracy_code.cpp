#include <iostream>
#include <cmath>

// 1. Include the Tracy header
#include "Tracy.hpp" 

using namespace std;

double slowFunction() {
    // 2. Add the profiling macro at the top of the function
    ZoneScoped; 
    
    double result = 0;
    for (long i = 0; i < 500000000; i++)
        result += sin(i) * cos(i/2.0);
    return result;
}

int main() {
    // Optional: Profile the main function as well
    ZoneScoped;
    
    cout << "Result: " << slowFunction() << endl;
    
    // Optional: Call FrameMark at the end of the loop/frame
    FrameMark; 
    
    return 0;
}
