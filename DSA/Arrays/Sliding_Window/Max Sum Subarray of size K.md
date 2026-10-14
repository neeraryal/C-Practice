# GFG – Max Sum Subarray of size K
> Size `k` wale subarrays mein maximum sum return karo

| Pattern | Data structure | Time | Space |
|---|---|---|---|
| [Fixed sliding window](sliding_window.md) | ek index `i` + running sum (C++) | O(n) | O(1) |

**How it works**
- **Soch:** [643](<643. Maximum Average Subarray.md>) wala hi idea: agli window = pichhli − `arr[i-k]` + `arr[i]`. Alag `left` nahi hai, `i-k` hi window ka left end hai.
- **Build:** `i < k` tak sirf `sum += arr[i]` — pehli window ban rahi hai.
- **Slide:** `i >= k` pe `sum -= arr[i-k]` (shrink) aur `sum += arr[i]` (expand) → size hamesha `k`.
- **Record:** Har `i` pe `max_sum = max(max_sum, sum)` (yahi line buggy hai, neeche dekho).
- **Kyun sahi hai:** `i = k-1 .. n-1` pe har size-`k` window ek baar banti hai, update O(1) → O(n).
- **Dry run:** `[100,200,300,400], k=2`
  - `i=1`: `[100,200]` = 300
  - `i=2`: 100 gaya, 300 aaya → 500
  - `i=3`: 200 gaya, 400 aaya → **700** ✔

**Gotcha:** `i < k` / `else` ka split zaroori hai — warna `arr[i-k]` negative index padhega. `max_sum` `INT_MIN` se start hota hai taaki negative sums bhi record ho sakein.
**vs related:** [643](<643. Maximum Average Subarray.md>) same window `left/right` ke saath, average return. [2461](<2461. Maximum Sum of Distinct Subarrays With Length K.md>) mein sum tabhi count hota hai jab window ke saare elements distinct hon.
**⚠ Known bug:** `max_sum` `i < k-1` pe bhi update hota hai, jab `sum` sirf partial window (size `< k`) ka hai. Statement mein positive numbers ki guarantee nahi hai → `[5,-10], k=2` pe `5` aata hai, `-5` chahiye. Fix: `if (i >= k-1) max_sum = max(max_sum, sum);`

<details><summary>Problem statement</summary>

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

</details>

## Code

```cpp
class Solution {
  public:
    int maxSubarraySum(vector<int>& arr, int k) 
    {
        int n=arr.size();
        int sum=0,max_sum=INT_MIN;  // INT_MIN sentinel → negative sums bhi record ho sakein
        for(int i=0; i<n;i++)
        {
            if(i<k)  // pehli window ban rahi hai → sirf add
            {
                sum+=arr[i];  // expand
            }
            else
            {
                sum-=arr[i-k];  // shrink: k peeche wala element bahar
                sum+=arr[i];  // expand: naya element andar → size k hi rehta hai
            }
            max_sum=max_sum>sum?max_sum:sum;  // ⚠ i < k-1 pe partial sum bhi record → negatives pe galat
        }
        return max_sum;
    }
};
```
