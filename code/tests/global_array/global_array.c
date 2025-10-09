// MiniC program to test global array
extern int print_int(int X);

// int global_arr[2][5];

// int function_with_array_param(int arr[10][5])
// {

// }
int a[10];
int b[10][10];

int init_arrays() {
    int i;
    int j;

    i = 0;
    j = 0;

    while (i < 10) {
        j = 0;
        while (j < 10)
        {
            b[i][j] = (i + 1) * (j + 1); // Example initialization
            j = j + 1;
        }
        a[i] = i + 1; // Example initialization
        i = i + 1;
    }

    return 0;
}

int vector_weighted_total(){
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
            total = total + a[i] * b[i][j]; 
            j = j + 1;
        }
        i = i + 1;
    }

    return total;
}
