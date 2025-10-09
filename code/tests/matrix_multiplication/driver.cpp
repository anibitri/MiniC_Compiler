#include <iostream>
#include <cstdio>

// clang++ driver.cpp addition.ll -o add
#define EPSILON 0.001

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
    int matrix_mul(float a[10][10], float b[10][10], float c[10][10], int n);
}

int  matrix_mul_test(float a[10][10], float b[10][10], float c[10][10], int n) {
    int i; 
    int j;
    int k;

    i = 0; // Initialize row index
    while (i < n) {
        j = 0;
        while (j < n) {
            k = 0;
            while (k < n) {
                c[i][j] = c[i][j] + (a[i][k] * b[k][j]);
                k = k + 1;
            }
            j = j + 1;
        }
        i = i + 1;
    }

    return 0;
}

bool verify_matrix(float c_1[10][10], float c_2[10][10], int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if ((c_1[i][j] - c_2[i][j]) > EPSILON || (c_2[i][j] - c_1[i][j]) > EPSILON) {
                std::cout << "Mismatch at (" << i << ", " << j << "): "
                          << c_1[i][j] << " != " << c_2[i][j] << std::endl;
                return false; // Matrices do not match
            }
        }
    }
    return true; // Matrices match
}

int main() {
    float arr1[10][10] = {{1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
                        {11, 12, 13, 14, 15, 16, 17, 18, 19, 20},
                        {21, 22, 23, 24, 25, 26, 27, 28, 29, 30},
                        {31, 32, 33, 34, 35, 36, 37, 38, 39, 40},
                        {41, 42, 43, 44, 45, 46, 47, 48, 49, 50},
                        {51, 52, 53, 54, 55, 56, 57, 58, 59, 60},
                        {61, 62, 63, 64, 65, 66, 67, 68, 69, 70},
                        {71, 72, 73, 74, 75, 76, 77, 78, 79, 80},
                        {81,82 ,83 ,84 ,85 ,86 ,87 ,88 ,89 ,90},
                        {91 ,92 ,93 ,94 ,95 ,96 ,97 ,98 ,99 ,100}};
    float arr2[10][10] = {10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
                        {20, 21, 22, 23, 24, 25, 26, 27, 28, 29},
                        {30, 31, 32, 33, 34, 35, 36, 37, 38, 39},
                        {40, 41, 42, 43, 44, 45, 46, 47, 48, 49},
                        {50,51 ,52 ,53 ,54 ,55 ,56 ,57 ,58 ,59},
                        {60 ,61 ,62 ,63 ,64 ,65 ,66 ,67 ,68 ,69},
                        {70 ,71 ,72 ,73 ,74 ,75 ,76 ,77 ,78 ,79},
                        {80 ,81 ,82 ,83 ,84 ,85 ,86 ,87 ,88 ,89},
                        {90 ,91 ,92 ,93 ,94 ,95 ,96 ,97 ,98 ,99},
                        {100, 101, 102, 103, 104, 105, 106, 107, 108, 109}};
    
    float c[10][10]; // Result matrix
    float c_test[10][10]; // Result matrix for testing
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            c[i][j] = 0.0f; // Initialize result matrix
            c_test[i][j] = 0.0f; // Initialize test matrix
        }
    }
    matrix_mul(arr1, arr2, c, 10);
    matrix_mul_test(arr1, arr2, c_test, 10);
    
    auto test_out = verify_matrix(c_test, c, 10);
    std::cout << "Matrix Multiplication Test: " << (test_out ? "PASSED" : "FAILED") << std::endl;
    if(test_out) 
      std::cout << "PASSED Result: " << std::endl;
  	else 
  	  std::cout << "FAILED Result: " << std::endl;
}