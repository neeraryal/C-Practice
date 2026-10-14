/**
 * Note: The returned array must be malloced, assume caller calls free().
 */
int* searchRange(int* arr, int n, int target, int* returnSize) 
{
    int * res_ret= malloc( sizeof (int )*2);
    *returnSize=2;
    int left=0 , right=n-1;
    int res=-1;

    //Find left most occurance
    while (left <= right)
    {
        int mid = left + (right-left)/2;
        if( arr[mid] == target)
        {
            res=mid;
            right= mid -1;
        }
        else if ( arr[mid] > target)
        {
            right= mid -1;
        }
        else
        left=mid+1;
    }

    res_ret[0]=res;

    //Find right most occurance
    left=0 ; right=n-1;
    
    while (left <= right)
    {
        int mid = left + (right-left)/2;
        if( arr[mid] == target)
        {
            res=mid;
            left= mid +1;
        }
        else if ( arr[mid] > target)
        {
            right= mid -1;
        }
        else
        left=mid+1;
    }

    res_ret[1]=res;

    return res_ret;
}