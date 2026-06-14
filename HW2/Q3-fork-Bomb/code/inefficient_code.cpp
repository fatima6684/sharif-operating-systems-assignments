#include <iostream>
#include <cmath>
using namespace std;

double slowFunction() {
    double result = 0;
    for (long i = 0; i < 500000000; i++)
        result += sin(i) * cos(i/2.0);
    return result;
}

int main() {
    cout << "Result: " << slowFunction() << endl;
    return 0;
}
