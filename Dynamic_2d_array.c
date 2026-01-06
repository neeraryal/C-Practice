#include <stdio.h>
#include <stdlib.h>
int main() {

    int **arr1 = malloc(sizeof(int*)*5); // array of int pointers
    
    arr1[0]= (int*)malloc(sizeof(int)*5); // each pointer points to an array of integers
    arr1[1]= (int*)malloc(sizeof(int)*5); // each pointer points to an array of integers
    arr1[2]= (int*)malloc(sizeof(int)*5); // each pointer points to an array of integers
    
    *(arr1+3)= (int*)malloc(sizeof(int)*5); // each pointer points to an array of integers
    *(arr1+4)= (int*)malloc(sizeof(int)*5); // each pointer points to an array of integers
    
    // arr1[3][4];
    *(*(arr1+3)+4)=12; // assigning value to the element at row 3, column 4
    *(arr1[2]+3)=13; // assigning value to the element at row 2, column 3
    (*(arr1+4))[4]=14; // assigning value to the element at row 4, column 4
    
    printf("%d \n",arr1[3][4]);
    printf("%d \n",arr1[2][3]);
    printf("%d \n",arr1[4][4]);
    return 0;
}
