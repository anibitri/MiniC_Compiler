// MiniC program to test addition
extern int print_int(int X);

// int global_arr[2][5];

// int function_with_array_param(int arr[10][5])
// {

// }

int arr_addition(int n, int m){
    int arr_result[10][5];
	int result;
    int idx;
    int temp;
    arr_result[0][0] = m;
    arr_result[0][1] = n;
    // idx = 0;
    temp = arr_result[0][0] + arr_result[0][1];
	// result = n;
    
    // arr_result[0][0] = n;
    // print_int(arr_result[0][0]);
    // if(n == 4) {
    //     print_int(n+m);
    // }
    // else {
    //     print_int(n*m);
    // }

    return temp;
}
