/*
Given an integer array nums sorted in non-decreasing order, return an array of the squares of each number sorted in non-decreasing order.

 

Example 1:

Input: nums = [-4,-1,0,3,10]
Output: [0,1,9,16,100]
Explanation: After squaring, the array becomes [16,1,0,9,100].
After sorting, it becomes [0,1,9,16,100].
Example 2:

Input: nums = [-7,-3,2,3,11]
Output: [4,9,9,49,121]
*/

/* =========================
    Solution 1 by using 2 pointer and splitting array 
   ========================= */

int* sortedSquares(int* arr, int n, int* returnSize) 
{
    int left_index=0,i=0;
    while(left_index<n && arr[left_index]<0){left_index++;}
    i=left_index;
    left_index-=1;

    int * res= calloc(n,sizeof(arr));
    *returnSize=n;
    int res_index=0;
    while(left_index>=0 && i<n)
    {
        if(abs(arr[left_index])<arr[i])
        {
            res[res_index++]=arr[left_index]*arr[left_index];
            left_index--;
        }
        else
        {
            res[res_index++]=arr[i]*arr[i];
            i++;
        }
    }
    if(i!=n)
    {
        while(i<n)
        {
            res[res_index++]=arr[i]*arr[i];
            i++;
        }

    }

    if(left_index>=0)
    {
        while(left_index>=0)
        {
            res[res_index++]=arr[left_index]*arr[left_index];
            left_index--;
        }
    }
    return res;
    
}



/* =========================
    Solution 2 by using 2 pointer --Optimized
   ========================= */

int* sortedSquares(int* arr, int n, int* returnSize) {
    *returnSize=n;
    int* res= calloc (n, sizeof(int));
    int left= 0 , right = n-1, index=n-1;

    while(left<=right)
    {
        if(abs(arr[left])>abs(arr[right]))
        {
            res[index--]=arr[left]*arr[left];
            left++;
        }
        else
        {
            res[index--]=arr[right]*arr[right];
            right--;
        }
    }
    return res;
}