// MiniC program to test addition
extern int print_int(int X);

// int global_arr[2][5];

// int function_with_array_param(int arr[10][5])
// {

// }

int vector_total(int a[10], int b[10], int n){
    int i;
    int total;
    i = 0;

    while (i < n) {
        total = total + a[i] + b[i];
        i = i + 1;
    }

    return total;
}
