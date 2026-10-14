# GFG – Smallest sum contiguous subarray
> Contiguous subarray (kam se kam 1 element) ka minimum sum return karo

| Pattern | Data structure | Time | Space |
|---|---|---|---|
| [Kadane – min sum, reset on ≥ 0](kadane.md) | `vector<int>`, 2 variables (`sum`, `res`) | O(n) | O(1) |

**How it works**
- **Soch:** [53](<53. Maximum Subarray(Kadanes).md>) ka mirror. Minimum chahiye, to positive running `sum` aage wale elements ko sirf upar kheenchega → drop karo.
- **Expand:** `sum += arr[right]` — current element ko running subarray mein jodo.
- **Record answer:** `res = min(res, sum)` — reset se pehle, taaki all-positive array mein smallest element pakda jaaye.
- **Reset:** `sum >= 0` → `sum = 0`.
- **Kyun sahi hai:** Min-sum subarray ka koi prefix positive nahi ho sakta (warna use hata ke sum aur chhota hota) → positive prefix drop karna safe. Ek pass → O(n), O(1).
- **Dry run:** `{3,-4,2,-3,-1,7,-5}`
  - `3`: sum=3, res=3 → reset
  - `-4,2,-3,-1`: sum -4→-2→-5→-6, res=-6
  - `7`: sum=1 → reset; `-5`: sum=-5, res -6 hi rehta → `-6` ✔

**Gotcha:** `res = arr[0]` se start — `{2,6,8,1,4}` pe answer `1` hai, 0 nahi. `left` unused hai, aur function ka naam `smallestSumSubarray` hai jabki task text `smallestSubarraySum()` bolta hai — GFG jo signature de wahi rakhna.
**vs related:** [53](<53. Maximum Subarray(Kadanes).md>) se bas do jagah farak: `>` ki jagah `<`, aur reset `sum <= 0` ki jagah `sum >= 0` pe.

<details><summary>Problem statement</summary>

Given an array arr[] of N integers. Find the contiguous sub-array(containing at least one number) which has the minimum sum and return its sum.

Example 1:

Input: 
arr[] = {3,-4, 2,-3,-1, 7,-5}
Output: -6
Explanation: sub-array which has smallest 
sum among all the sub-array is {-4,2,-3,-1} = -6
Example 2:

Input:
arr[] = {2, 6, 8, 1, 4}
Output: 1
Explanation: sub-array which has smallest
sum among all the sub-array is {1} = 1
Your Task:
You don't need to read input or print anything. The task is to complete the function smallestSubarraySum() which takes arr[] and N as input parameters and returns the sum of subarray with minimum sum.

</details>

## Code

```cpp
class Solution {
  public:
    int smallestSumSubarray(vector<int>& arr) 
    {
        int n=arr.size();
        int sum=0,res=arr[0];  // res = arr[0] → all-positive array mein bhi sahi min
        int left =0, right=0;  // left unused — sirf right se scan hota hai
    
        while(right<n)
        {
            sum+=arr[right];  // expand: arr[right] ko current subarray mein jodo
            res=res<sum?res:sum;  // record answer (min) — reset se pehle
            if(sum>=0)  // sum ≥ 0 → ye prefix min ko sirf bada karega
            {
                sum=0;  // reset: agle element se naya subarray
            }
            right++;
        }
        return res;
    }
};

```
