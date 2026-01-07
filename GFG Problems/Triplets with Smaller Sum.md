Given an array arr[] of distinct integers of size n and a value sum, the task is to find the count of triplets (i, j, k), having (i<j<k) with the sum of (arr[i] + arr[j] + arr[k]) smaller than the given value sum.


Examples :


Input: n = 4, sum = 2, arr[] = {-2, 0, 1, 3}
Output:  2
Explanation: Below are triplets with sum less than 2 (-2, 0, 1) and (-2, 0, 3). 
Input: n = 5, sum = 12, arr[] = {5, 1, 3, 4, 7}
Output: 4
Explanation: Below are triplets with sum less than 12 (1, 3, 4), (1, 3, 5), (1, 3, 7) and (1, 4, 5).
Expected Time Complexity: O(N2).
Expected Auxiliary Space: O(1).

```c
int sort_cmp(const void* i, const void* j)
{
    long long a =*(const long long *)i;
    long long b =*(const long long *)j;
    
    if (a<b) return -1;
    if (a>b) return 1;
    return 0;
}

class Solution {

  public:
    long long countTriplets(int n, long long target, long long arr[]) 
    {
        qsort(arr,n,sizeof(long long),sort_cmp);
        // sort(arr,arr+n);
        long long res=0;
        
        for(int i=0;i<n-2;i++)
        {
            int left =i+1;
            int right= n-1;

            while(left<right)
            {
                long long sum= arr[i]+ arr[left] + arr[right];
                if(sum<target)
                {
                    res+=(right-left);
                    left++;
                }
                else
                {
                    right--;
                }
            }
        }
        return res;
    }
};
```