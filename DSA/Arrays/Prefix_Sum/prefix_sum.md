# Prefix Sum Pattern

**Core idea:** Har index pe "ab tak ka" aggregate (sum / product / max) rakh lo, taaki koi bhi range ka answer O(1) mein mile: `sum(l..r) = prefix[r] - prefix[l-1]`. Left se prefix aur right se suffix — dono mila ke "mere alawa sab" ya "mera left vs mera right" wale sawaal ek-do pass mein. Subarrays count karne hon to prefix sums hash map mein rakho (prefix sums pe Two Sum).

**Signals**
- "Left sum == right sum", "product of all except self"
- "Subarray sum == k" aur negatives bhi hain (sliding window fail)
- Bar-bar range sum / range product chahiye
- Har element ko "left mein sab" ya "right mein sab" se compare karna (prefix max / suffix min)

## Template
```c
/* Running prefix sum + hash map: sum == k wale subarrays count karo */
int sum = 0, res = 0;
insert(0, 1);                        /* empty prefix → index 0 se shuru wale subarrays */
for (int i = 0; i < n; i++) {
    sum += arr[i];                   /* prefix = arr[0..i] */
    res += count_of(sum - k);        /* pehle ke jitne prefix == sum-k → utne subarrays */
    insert(sum, 1);                  /* search ke BAAD insert */
}

/* Prefix × suffix: left pass array mein, right pass running variable se */
pre[0] = ID;  for (i = 1; i < n; i++) pre[i] = op(pre[i-1], arr[i-1]);
suf = ID;     for (i = n-1; i >= 0; i--) { res[i] = op(pre[i], suf); suf = op(suf, arr[i]); }
```

## Progression
| # | Problem | What changes | New intuition | Key code delta |
|---|---|---|---|---|
| 1 | [724. Find Pivot Index](<724. Find Pivot Index.md>) | Left sum == right sum | Total ek baar nikalo → `right = total - left - arr[i]`; prefix array bhi nahi chahiye | `if(l_sum+arr[i] == t_sum -l_sum) return i;` → `l_sum+=arr[i];` |
| 2 | [238. Product of Array Except Self](<238. Product of array Except itsel.md>) | Sum → product, division mana | `total / arr[i]` nahi kar sakte → left products array + right se running suffix | `right_product_prefix[i]=arr[i-1]*right_product_prefix[i-1];` + `res[res_index--]=left_product*right_product_prefix[i]; left_product*=arr[i];` |
| 3 | [560. Subarray Sum Equals K](<560.Subarray Sum Equals K.md>) | Ek index nahi, saare subarrays count; negatives | `prefix[r] - prefix[l-1] = k` → `sum - k` wale purane prefixes ginno (Two Sum on prefix sums) | `search(sum-k)` → `res+=*search_index;` + `insert(sum,1);` |
| 4 | [581. Shortest Unsorted Continuous Subarray](<581. Shortest Unsorted Continuous Subarray.md>) | Sum ki jagah running max/min; boundary dhoondhni hai | Prefix max se chhota = out of place (right boundary), suffix min se bada = out of place (left boundary) | `if(arr[left]<max_seen) right_boundary=left; else max_seen=arr[left];` |

## Gotchas
- Order: answer pehle check/likho, phir `arr[i]` running value mein add karo (724 `l_sum`, 238 `left_product`) — warna `arr[i]` apne hi answer mein aa jaata hai.
- 560 mein empty prefix (sum 0) mat bhoolo: `if(sum == k) res++;` ya map ko `{0:1}` se seed karo. Aur search pehle, insert baad mein (`k=0` pe warna empty subarray ginega).
- Identity sahi rakho: sum → `0`, product → `1`, max → `INT_MIN`, min → `INT_MAX`.
- Negatives hon to sliding window nahi chalta (sum monotonic nahi) → prefix sum + hash map.
- 238 mein naam ulte hain (`right_product_prefix` = left products, `left_product` = suffix) — naam pe nahi, formula pe dhyaan do.

## Related elsewhere
- [1. Two Sum](<../Two_Sum/1. Two_Sum_Problem.md>) — 560 ka complement lookup yahi hai, bas values ki jagah prefix sums.
- [Kadane](<../Kadane/kadane.md>) — max subarray sum = `prefix[r] - min(prefix[l])`; Kadane yahi running form mein karta hai.
- [Sliding Window](<../Sliding_Window/sliding_window.md>) — sab elements positive hon to subarray-sum wale sawaal window se O(1) space mein ([209](<../Sliding_Window/209. Minimum Size Subarray Sum.md>)).
