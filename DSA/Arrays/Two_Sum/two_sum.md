# Two Sum Pattern

**Core idea:** Har element `x` ka partner fixed hai: `target - x`. Pura game partner ko fast dhoondhne ka hai. Array unsorted hai (aur indices chahiye) to hash map. Array sorted hai (ya sort kar sakte ho) to opposite-end two pointers. 3 ya zyada numbers ho to ek element fix karo aur baaki pe Two Sum chalao.

**Signals**
- "Pair / triplet jiska sum `==` / closest / `<` target"
- Array sorted hai, ya sirf values/count chahiye (to sort kar sakte ho)
- O(n²) / O(n³) brute force ko ek level neeche laana hai

## Template
```c
/* Sorted array: opposite-end pointers */
int left = 0, right = n - 1;
while (left < right) {
    int sum = arr[left] + arr[right];
    if (sum == target)     { /* found */ break; }
    else if (sum < target) left++;    /* sum badhana hai */
    else                   right--;   /* sum ghatana hai */
}
/* k-Sum: for (i...) ek element fix karo, baaki pe yahi loop with target - arr[i] */
```

## Progression
| # | Problem | What changes | New intuition | Key code delta |
|---|---|---|---|---|
| 1 | [1. Two Sum](<1. Two_Sum_Problem.md>) | Unsorted, indices return karne hain | Sort nahi kar sakte (indices bigdenge) → `target - x` ko hash map mein dhoondho | `search(target - arr[i])` pehle, `insert(arr[i], i)` baad mein |
| 2 | [167. Two Sum II](<167. Two Sum II - Input Array Is Sorted.md>) | Array sorted, O(1) space | Sum dekh ke pata chalta hai kaunsa end bekaar hai → use discard karo | `sum > target ? right-- : left++` |
| 3 | [15. 3Sum](<15. 3Sum.md>) | 3 numbers, sum 0, unique triplets | Ek `arr[i]` fix → baaki 167 with `target = -arr[i]`. Sort se duplicates paas aate hain → skip | `if (i > 0 && arr[i] == arr[i-1]) continue;` + match ke baad dup skip |
| 4 | [16. 3Sum Closest](<16. 3Sum Closest.md>) | Exact nahi, closest sum | Har sum candidate hai → min `abs(target - sum)` track karo. Uniqueness ki zarurat nahi | `if (abs(target - sum) < diff) res_sum = sum;` |
| 5 | [GFG Triplets with Smaller Sum](<Triplets with Smaller Sum.md>) | Count karna hai, `sum < target` | `sum < target` → `(left, right]` ka har `right` chalega → ek saath count | `res += right - left; left++;` |

## Gotchas
- Indices chahiye (1) → sort mat karo. Sirf values/count chahiye (15, 16, GFG) → sort karo.
- Hash map mein **search pehle, insert baad mein** — warna element khud ke saath pair bana lega.
- 167 ka answer 1-indexed hai → `left+1, right+1`.
- 3Sum mein duplicates teen jagah skip: `i` pe, aur match ke baad `left`/`right` pe.

## Related elsewhere
- [560. Subarray Sum Equals K](<../Prefix_Sum/560.Subarray Sum Equals K.md>) — prefix sums pe Two Sum: kya `prefix - k` pehle dekha hai?
- [Two Pointers](<../Two_Pointers/two_pointers.md>) — opposite-end pointers ke aur uses (977, 11).
- [Hashing](<../Hashing/hashing.md>) — hash set/map ke aur uses (217, 128, 347).
