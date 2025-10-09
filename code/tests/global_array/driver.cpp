#include <iostream>
#include <cstdio>

// clang++ driver.cpp addition.ll -o add

#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

extern "C" DLLEXPORT int print_int(int X) {
  fprintf(stderr, "%d\n", X);
  return 0;
}

extern "C" DLLEXPORT float print_float(float X) {
  fprintf(stderr, "%f\n", X);
  return 0;
}

extern "C" {
    int vector_weighted_total();
}

int t_a[10];
int t_b[10][10];

int init_arrays() {
    int i;
    int j;

    i = 0;
    j = 0;

    while (i < 10) {
        j = 0;
        while (j < 10)
        {
            t_b[i][j] = (i + 1) * (j + 1); // Example initialization
            j = j + 1;
        }
        t_a[i] = i + 1; // Example initialization
        i = i + 1;
    }

    return 0;
}

int vector_weighted_total_test(){
    int total;
    int i;
    int j;
    
    init_arrays(); // Initialize the arrays before using them
    i = 0;
    j = 0;
    total = 0; // Initialize total to 0

    while (i < 10) {
        j = 0;

        while (j < 10)
        {
            total = total + t_a[i] * t_b[i][j]; 
            j = j + 1;
        }
        i = i + 1;
    }

    return total;
}


int main() {
    // Initialize the arrays
    init_arrays();
    
    if(vector_weighted_total() == vector_weighted_total_test()) 
        std::cout << "PASSED Result: " << vector_weighted_total() << std::endl;
    else 
        std::cout << "FAILED Result: " << vector_weighted_total() << " expected: " << vector_weighted_total_test()<< std::endl;
}