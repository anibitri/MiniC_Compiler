#include <iostream>
#include <cstdio>

// clang++ driver.cpp leap.ll -o leap


#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

extern "C" DLLEXPORT int print_int(int X) {
  fprintf(stderr, "%d\n", X);
  return 0;
}

extern "C" {
    int is_leap(int year);
}

int main() {
    
    int year = 2000;
    int result = is_leap(year);
    if( result == 1) 
      std::cout << "PASSED Result: " << result << " for year: " << year << std::endl;
    else 
      std::cout << "FALIED Result: " << result << " for year: " << year << std::endl;

    year = 1900;
    result = is_leap(year);
    if( result == 0) 
      std::cout << "PASSED Result: " << result << " for year: " << year << std::endl;
    else 
      std::cout << "FALIED Result: " << result << " for year: " << year << std::endl;

    year = 2012;
    result = is_leap(year);
    if( result == 1) 
      std::cout << "PASSED Result: " << result << " for year: " << year << std::endl;
    else 
      std::cout << "FALIED Result: " << result << " for year: " << year << std::endl;

    year = 2019;
    result = is_leap(year);
    if( result == 0) 
      std::cout << "PASSED Result: " << result << " for year: " << year << std::endl;
    else 
      std::cout << "FALIED Result: " << result << " for year: " << year << std::endl;

    return 0;
}
