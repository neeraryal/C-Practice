# GFG – Triplets with Smaller Sum
> Aise triplets `(i<j<k)` ka count do jinka sum `target` se chhota ho

| Pattern | Data structure | Time | Space |
|---|---|---|---|
| [Two Sum – counting with two pointers](two_sum.md) | None | O(n²) | O(1) |

**How it works**
- **Soch:** Sort + `i` fix + two pointers. Lekin yahan count chahiye. Trick: jab `arr[i]+arr[left]+arr[right] < target`, to `right` ki jagah `left+1 … right` mein se koi bhi lo — sum aur chhota hi hoga. Matlab ye saare `right - left` triplets ek saath valid hain.
- **Move:** `sum < target` → `res += right - left`, phir `left++`. Warna sum bada hai → `right--`.
- **Kyun sahi hai:** Har `(i, left)` ke saare valid `right` ek hi step mein count ho gaye, ek-ek ginne ki zarurat nahi → O(n³) se O(n²). Sort se indices ka order badalta hai, par triplets ka set wahi rehta hai, isliye count same.
- **Dry run:** `{-2,0,1,3}, sum=2`
  - `i=-2`: `-2+0+3=1 < 2` → `res += 3-1 = 2` → `(-2,0,1)`, `(-2,0,3)`
  - `-2+1+3=2` not `< 2` → `right--` → stop
  - `i=0`: `0+1+3=4` → `right--` → stop → **2** ✔

**Gotcha:** Array `long long` hai to comparator bhi `long long` padhna chahiye (int comparator galat sort karega). Code C++ hai (`class Solution`). `qsort` caller ka array badal deta hai.
**vs related:** [15](<15. 3Sum.md>)/[16](<16. 3Sum Closest.md>) mein `sum == target` special tha; yahan sirf `<` matter karta hai, aur ek step mein kai answers count hote hain.

<details><summary>Problem statement</summary>

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

</details>

## Code

```cpp
int sort_cmp(const void* i, const void* j)
{
    long long a =*(const long long *)i;  // array long long hai → comparator bhi long long padhe
    long long b =*(const long long *)j;
    
    if (a<b) return -1;
    if (a>b) return 1;
    return 0;
}

class Solution {

  public:
    long long countTriplets(int n, long long target, long long arr[]) 
    {
        qsort(arr,n,sizeof(long long),sort_cmp);  // sort → indices ka order badla, par triplets ka count same rehta hai
        // sort(arr,arr+n);
        long long res=0;
        
        for(int i=0;i<n-2;i++)
        {
            int left =i+1;  // arr[i] fix, baaki pe two pointers
            int right= n-1;

            while(left<right)
            {
                long long sum= arr[i]+ arr[left] + arr[right];
                if(sum<target)
                {
                    res+=(right-left);  // right ko left+1..right mein kahin bhi lao, sum aur chhota → (right-left) triplets ek saath
                    left++;  // is left ke saare triplets gin liye → next left
                }
                else
                {
                    right--;  // sum >= target → bada element hatao
                }
            }
        }
        return res;
    }
};
```
