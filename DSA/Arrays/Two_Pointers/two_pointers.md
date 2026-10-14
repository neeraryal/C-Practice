# Two Pointers Pattern

**Core idea:** Ek hi array pe do index chalao, taaki nested loop (O(n²)) ki jagah ek pass (O(n)) mein kaam ho jaaye. Do flavours hain. **Same direction (read/write):** `read` har element dekhta hai, `write` batata hai ki "rakhne layak" part kahan tak bana — in-place filter / partition ke liye. **Opposite ends:** `left`/`right` andar ki taraf aate hain, aur har step pe ek end ko proof ke saath discard karte hain (sorted order ya "bottleneck" argument).

**Signals**
- "In-place", "O(1) extra space", "relative order same rakho" → read/write pointers
- Sorted array se duplicates hatao / har value max k baar rakho
- Array mein sirf 2–3 distinct values (0/1, 0/1/2) → partition
- Sorted array jiske dono ends pe extreme values hon (squares, pair sum) → opposite ends
- "Max area / best pair" jahan ek end ko safely discard kar sakte ho → opposite ends greedy

## Template
```c
/* Same direction: read/write (in-place filter) */
int write = 0;
for (int read = 0; read < n; read++) {
    if (keep(arr[read]))              /* rakhna hai? */
        arr[write++] = arr[read];     /* kept part ke end pe likho (order chahiye to swap) */
}
return write;                         /* kept elements ka count */

/* Opposite ends */
int left = 0, right = n - 1;
while (left < right) {                /* har element place karna ho (977) to <= */
    /* arr[left], arr[right] se answer update */
    if (left_end_useless) left++;     /* jo end ab kisi kaam ka nahi, use hatao */
    else                  right--;
}
```

## Progression
| # | Problem | What changes | New intuition | Key code delta |
|---|---|---|---|---|
| 1 | [26. Remove Duplicates from Sorted Array](<26. Remove Duplicates from Sorted Array.md>) | Sorted array, har value ek baar | Read/write: sorted hai to naya value sirf last kept value se compare karo | `if(arr[i]!= arr[unique_index])` → `unique_index++; arr[unique_index]=arr[i];` |
| 2 | [80. Remove Duplicates from Sorted Array II](<80. Remove Duplicates from Sorted Array II.md>) | Har value max 2 baar | Kept part ke 2 peeche wale se compare — written part se, original se nahi | `int index=2;` + `if(arr[i]!=arr[index-2]) arr[index++]=arr[i];` |
| 3 | [283. Move Zeroes](<283. Move all zeros.md>) | Sorted nahi; zeros end mein, order same | Copy ki jagah swap → zeros apne aap peeche chale jaate hain | `if(arr[j]!=0)` → `if(i!=j) swap(&arr[i], &arr[j]); i++;` |
| 4 | [GFG Segregate 0s and 1s](<Segregate 0s and 1s.md>) | Sirf 0/1, zeros left mein | 2-way partition = 283 ki ulti condition; `zero_index` 0-region ka end (`-1` se) | `if(arr[i]==0)` → `zero_index++; swap(arr[i], arr[zero_index]);` |
| 5 | [75. Sort Colors](<75. Sort Colors(sort 0,1,2).md>) | 3 values 0/1/2 | 3-way partition (Dutch flag): teesra pointer `right` 2s ko end mein; right se aaya element unknown → `i` mat badhao | `while ( i<= right )` + `else if (arr[i]==2) { swap(arr+i,arr+right); right --; }` |
| 6 | [977. Squares of a Sorted Array](<977. Squares of a Sorted Array.md>) | Opposite ends; negatives ke squares | Sabse bada square kisi ek end pe → `abs` compare, result **peeche se** bharo | `if(abs(arr[left])>abs(arr[right])) res[index--]=arr[left]*arr[left];` |
| 7 | [11. Container With Most Water](<11.Container With Most Water.md>) | Opposite ends; max area, kuch fill nahi karna | Greedy: chhoti line bottleneck hai → use hatao, badi wali se kabhi better nahi milega | `if(arr[left]<=arr[right]) left++; else right--;` |

## Gotchas
- XOR / add-sub swap same address pe value 0 kar dete hain → `i!=j` (283) / `a!=b` (75) guard zaroori. Temp-variable swap mein ye problem nahi.
- "At most k" (80) mein compare **written** part `arr[index-k]` se karo, original `arr[i-k]` se nahi — woh overwrite ho chuka ho sakta hai.
- 75 mein `right` se swap ke baad `i` mat badhao (aaya element unchecked hai), aur loop `i <= right` rakho.
- Loop condition: pair chahiye (11) → `left < right`; har element place karna hai (977) → `left <= right`.
- Return value dhyaan se: 26 mein write pointer last index hai → `unique_index+1`; 80 mein next slot hai → `index`.

## Related elsewhere
- [Two Sum](<../Two_Sum/two_sum.md>) — opposite pointers sorted array pe sum ke liye ([167](<../Two_Sum/167. Two Sum II - Input Array Is Sorted.md>), [15](<../Two_Sum/15. 3Sum.md>)).
- [83. Remove Duplicates from Sorted List](<../../Linked_List/Two_Pointers/83. Remove Duplicates from Sorted List.md>) — 26 hi hai, linked list pe (copy ki jagah `next` skip).
- [844. Backspace String Compare](<../../Strings/Two_Pointers/844. Backspace String Compare.md>) — do strings pe ek-ek pointer, peeche se scan.
