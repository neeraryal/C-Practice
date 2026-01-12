Given an array of integers arr[]  and a number k. Return the maximum sum of a subarray of size k.

Note: A subarray is a contiguous part of any given array.

Examples:

Input: arr[] = [100, 200, 300, 400], k = 2
Output: 700
Explanation: arr2 + arr3 = 700, which is maximum.
Input: arr[] = [1, 4, 2, 10, 23, 3, 1, 0, 20], k = 4
Output: 39
Explanation: arr1 + arr2 + arr3 + arr4 = 39, which is maximum.
Input: arr[] = [100, 200, 300, 400], k = 1
Output: 400
Explanation: arr3 = 400, which is maximum.

```cpp
class Solution {
  public:
    int maxSubarraySum(vector<int>& arr, int k) 
    {
        int n=arr.size();
        int sum=0,max_sum=INT_MIN;
        for(int i=0; i<n;i++)
        {
            if(i<k)
            {
                sum+=arr[i];
            }
            else
            {
                sum-=arr[i-k];
                sum+=arr[i];
            }
            max_sum=max_sum>sum?max_sum:sum;
        }
        return max_sum;
    }
};
```