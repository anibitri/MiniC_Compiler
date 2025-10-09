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
    int vector_total(int a[10], int b[10], int n);
}

int vector_total_test(int a[10], int b[10], int n) {
    int i;
    int total = 0; // Initialize total to 0
    i = 0;

    while (i < n) {
        total = total + (a[i] + b[i]);
        i = i + 1;
    }
    
    return total;
}

int main() {
    int arr1[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    int arr2[10] = {10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
    
    if(vector_total(arr1, arr2, 10) == vector_total_test(arr1, arr2, 10)) 

      std::cout << "PASSED Result: " << vector_total(arr1, arr2, 10) << std::endl;
  	else 
  	  std::cout << "FAILED Result: " << vector_total(arr1, arr2, 10) << std::endl;
}