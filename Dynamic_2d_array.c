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

########################################################################

// Online C compiler to run C program online
#include <stdio.h>
#include <stdlib.h>
int main() {
    
    int* arr =malloc(sizeof(int)*5);
    
    *(arr+0)=0;
    *(arr+1)=1;
    *(arr+2)=2;
    *(arr+3)=3;
    *(arr+4)=4;
    
    for(int i=0; i<5;i++)
    {
        printf(" Value at index %d is :%d\n",i,*(arr+i));
    }
    printf("\n\n");
    
    printf("Address of arr :%p \n", arr);
    printf("\n\n");
    for(int i=0; i<5;i++)
    {
        printf(" Area of index %d is :%p\n",i,(arr+i));
    }
    /*
    Address of arr :0x105332a0 

     Area of index 0 is :0x105332a0
     Area of index 1 is :0x105332a4
     Area of index 2 is :0x105332a8
     Area of index 3 is :0x105332ac
     Area of index 4 is :0x105332b0
    */
    printf("\n\n");
    
    //Accessing Values of array using pointer
    *(arr+0)=10;
    *(arr+1)=11;
    *(arr+2)=12;
    *(arr+3)=13;
    *(arr+4)=14;

    for(int i=0; i<5;i++)
    {
        printf(" Value at index %d is :%d\n",i,*(arr+i));
    }
    printf("\n\n");
    
    //Assigning arr start addres to some other pointer os same type and doing pointer arithematic
    
    int* p =arr+1;
    for(int i=0; i<4;i++)
    {
        printf(" Value at index %d is :%d\n",i+1,*(p+i));
    }
    printf("\n\n");
    
    *(p+0)=20;
    *(p+1)=21;
    *(p+2)=22;
    *(p+3)=23;
    *(p+4)=24;
    
    for(int i=0; i<4;i++)
    {
        printf(" Value at index %d is :%d\n",i,*(p+i));
    }
    printf("\n\n");

    return 0;
}