// MiniC program to test matrix multiply
extern float print_float(float X);

int matrix_mul(float a[10][10], float b[10][10], float c[10][10], int n)
{
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