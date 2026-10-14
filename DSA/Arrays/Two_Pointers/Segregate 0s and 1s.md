# GFG – Segregate 0s and 1s
> Sirf 0/1 wale array mein saare 0 left aur saare 1 right karo (in-place, O(n))

| Pattern | Data structure | Time | Space |
|---|---|---|---|
| [Two Pointers – 2-way partition](two_pointers.md) | `vector<int>` (in-place) | O(n) | O(1) |

**How it works**
- **Soch:** Ye [283](<283. Move all zeros.md>) ka mirror hai: wahan non-zero aage bheje, yahan 0 aage bhejo. `zero_index` = 0-region ka last index (shuru mein `-1` = region khaali).
- **Move:** `arr[i]==0` → `zero_index++`, phir `swap(arr[i], arr[zero_index])`. 1 mila → kuch nahi, bas `i` aage.
- **Kyun sahi hai:** Invariant: `arr[0..zero_index]` = sab 0, `arr[zero_index+1..i-1]` = sab 1. Naya 0 aaye to 1-region ka pehla 1 uski jagah chala jaata hai. Ek pass → O(n).
- **Dry run:** `[0,0,1,1,0]`
  - `i=0,1`: 0 → `zero_index` = 0, 1 (self-swap)
  - `i=2,3`: 1 → skip
  - `i=4`: 0 → `zero_index=2`, swap(arr[4], arr[2]) → `[0,0,0,1,1]` ✔

**Gotcha:** Custom `swap(int*, int*)` kabhi call hi nahi hota — `swap(arr[i], arr[zero_index])` `int&` pass karta hai, to `std::swap` chalta hai (pointer wala overload match hi nahi karta). Code C++ hai (`class Solution`, `vector`).
**vs related:** [75](<75. Sort Colors(sort 0,1,2).md>) mein 3 values → 3 regions, extra `right` pointer 2s ko end mein bhejta hai. [283](<283. Move all zeros.md>) same loop, condition ulti.

<details><summary>Problem statement</summary>

Given an array arr consisting of only 0's and 1's in random order. Modify the array in-place to segregate 0s onto the left side and 1s onto the right side of the array.

Examples :

Input: arr[] = [0, 0, 1, 1, 0]
Output: [0, 0, 0, 1, 1]
Explanation:  After segregation, all the 0's are on the left and 1's are on the right. Modified array will be [0, 0, 0, 1, 1].
Input: arr[] = [1, 1, 1, 1]
Output: [1, 1, 1, 1]
Explanation: There are no 0s in the given array, so the modified array is [1, 1, 1, 1]
Expected Time Complexity: O(n)
Expected Auxiliary Space: O(1)

</details>

## Code

```cpp
// User function template for C++

void swap(int * a, int * b )  // kabhi call nahi hota: neeche int& pass hote hain → std::swap
{
    *a= *a + *b;
    *b= *a - *b;
    *a= *a - *b;
}

class Solution {
  public:
    void segregate0and1(vector<int> &arr) 
    {
        int n= arr.size();
        int zero_index=-1;  // 0-region ka last index; -1 = abhi koi 0 nahi
        for(int i=0 ;i<n;i++)
        {
            if(arr[i]==0)  // 0 mila → left wale 0-region mein bhejna hai
            {
                zero_index++;  // region badhao: is slot pe ya to 1 hai ya khud i
                swap(arr[i], arr[zero_index]);  // std::swap → 0 left mein, wahan ka 1 i pe
            }
        }
    }
};
```
