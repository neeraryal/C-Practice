/*
Given an integer array nums, move all 0's to the end of it while maintaining the relative order of the non-zero elements.

Note that you must do this in-place without making a copy of the array.

 

Example 1:

Input: nums = [0,1,0,3,12]
Output: [1,3,12,0,0]
Example 2:

Input: nums = [0]
Output: [0]

*/
void swap(int * a, int *b)
{
    *a= *a ^ *b;
    *b= *a ^ *b;
    *a= *a ^ *b;
}

void moveZeroes(int* arr, int n) 
{
    int i=0, j=0;

    while(j<n)
    {
        if(arr[j]!=0) //if(arr[j]==0) will move all 0 to left 
        {
            if(i!=j)
            {swap(&arr[i], &arr[j]);}
            i++;
        }
        j++;
    }
}