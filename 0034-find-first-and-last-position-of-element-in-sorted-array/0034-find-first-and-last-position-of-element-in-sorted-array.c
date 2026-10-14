/**
 * Note: The returned array must be malloced, assume caller calls free().
 */
int* searchRange(int* arr, int n, int target, int* returnSize) 
{
    // result array heap pe banaya (caller free karega), [first, last] store hoga
    int * res_ret = malloc(sizeof(int) * 2);
    *returnSize = 2;  // LeetCode ko batana hai ki array ka size 2 hai

    int left = 0, right = n - 1;
    int res = -1;  // -1 = target mila hi nahi (default)

    // ---- Leftmost occurrence dhundo ----
    while (left <= right)
    {
        // (left+right)/2 me overflow ho sakta hai, isliye ye safe formula
        int mid = left + (right - left) / 2;

        if (arr[mid] == target)
        {
            res = mid;         // ek match mila, isko save kar lo
            right = mid - 1;   // par aur left me dekho, shayad pehle bhi ho
        }
        else if (arr[mid] > target)
        {
            right = mid - 1;   // mid bada hai, left half me jao
        }
        else
            left = mid + 1;    // mid chhota hai, right half me jao
    }

    res_ret[0] = res;  // first index (ya -1 agar nahi mila)

    // ---- Rightmost occurrence dhundo ----
    // search range reset karo, pehle wali binary search ne left/right bigaad diye the
    left = 0; right = n - 1;

    while (left <= right)
    {
        int mid = left + (right - left) / 2;

        if (arr[mid] == target)
        {
            res = mid;         // match mila, save karo
            left = mid + 1;    // ab aur right me dekho, shayad last occurrence aage ho
        }
        else if (arr[mid] > target)
        {
            right = mid - 1;
        }
        else
            left = mid + 1;
    }

    res_ret[1] = res;  // last index (ya -1)

    return res_ret;
}