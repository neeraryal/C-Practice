# Sliding Window Pattern

**Core idea:** Contiguous subarray ke liye brute force har `(i, j)` pair check karta hai → O(n²). Sliding window mein `right` naya element andar laata hai (expand), `left` purana bahar nikalta hai (shrink), aur window ki state (sum / count / freq) O(1) mein update hoti hai — dobara calculate nahi. Dono pointers sirf aage jaate hain → O(n).
**Fixed window:** size `k` diya hai → har step pe ek andar, ek bahar, `if` kaafi. **Variable window:** size khud dhoondhna hai → `while` se shrink. Ye tabhi chalta hai jab condition *monotonic* ho (window badi → sum / zeros / distinct badhte hi hain), isliye positive numbers ya counts chahiye.

**Signals**
- "Subarray of size `k`" → fixed window
- "Longest / shortest subarray jisme sum `>=` target / at most `k` zeros / at most `k` distinct" → variable window
- "Kitne subarrays jinka product `<` k" → variable window + count
- Condition monotonic hai: elements positive, ya count/freq based. Negative numbers + `sum == k` → window nahi, prefix sum

## Template
```c
/* Fixed window (size k) */
for (right = 0; right < n; right++) {
    add(arr[right]);                                   /* expand */
    if (right - left + 1 > k) remove(arr[left++]);     /* size k+1 → ek bahar */
    if (right - left + 1 == k) record();               /* har size-k window yahan */
}

/* Variable — MAXIMIZE (longest valid): shrink jab tak INVALID, record BAAD mein */
for (right = 0; right < n; right++) {
    add(arr[right]);
    while (invalid()) remove(arr[left++]);
    res = max(res, right - left + 1);                  /* window ab valid hai */
}

/* Variable — MINIMIZE (shortest valid): shrink jab tak VALID, record ANDAR */
for (right = 0; right < n; right++) {
    add(arr[right]);
    while (valid()) {
        res = min(res, right - left + 1);              /* shrink se pehle record */
        remove(arr[left++]);
    }
}

/* Variable — COUNT: maximize wala loop, bas record ki jagah res += right - left + 1 */
```

## Progression
| # | Problem | What changes | New intuition | Key code delta |
|---|---|---|---|---|
| 1 | [643. Maximum Average Subarray I](<643. Maximum Average Subarray.md>) | Fixed size `k`, max average | Agli window = pichhli − ek + ek → running sum, poora dobara jodna nahi | `avg += arr[right];` + `if (right - left + 1 > k) { avg -= arr[left]; left++; }` |
| 2 | [GFG Max Sum Subarray of size K](<Max Sum Subarray of size K.md>) | Average ki jagah sum, `left` pointer nahi | Left end = `i - k` → ek index kaafi. Record sirf poori window pe hona chahiye (⚠ code partial window bhi record karta hai) | `sum -= arr[i-k]; sum += arr[i];` |
| 3 | [2461. Maximum Sum of Distinct Subarrays With Length K](<2461. Maximum Sum of Distinct Subarrays With Length K.md>) | Fixed window + "saare elements distinct" | State sirf sum nahi — `freq` + `distinct` bhi window ke saath slide → validity O(1) mein | `if (freq[arr[right]]==0) distinct++;` + `if (right-left+1==k && distinct==k)` |
| 4 | [485. Max Consecutive Ones](<485. Max Consecutive Ones.md>) | Size fixed nahi — longest run of 1s | Precursor: koi 0 allowed nahi → 0 pe poori window reset, `left` ki zarurat hi nahi | `else one_count = 0;` |
| 5 | [1004. Max Consecutive Ones III](<1004. Max Consecutive Ones III.md>) | `k` zeros allowed | Reset nahi kar sakte → **variable window, maximize**: shrink jab tak INVALID, record baad mein | `while (zero_count > k) { if (arr[left]==0) zero_count--; left++; }` |
| 6 | [209. Minimum Size Subarray Sum](<209. Minimum Size Subarray Sum.md>) | Longest nahi, shortest valid chahiye | **Minimize**: shrink jab tak VALID, record shrink loop ke ANDAR | `while (sum >= target) { min_window = min(...); sum -= arr[left]; left++; }` |
| 7 | [713. Subarray Product Less Than K](<713. Subarray Product Less Than K.md>) | Ek best window nahi, saare valid subarrays count | Valid `[left..right]` → `right` pe khatam har subarray valid → ek saath `right-left+1` | `res += (right - left + 1);` + `if (k <= 1) return 0;` |
| 8 | [904. Fruit Into Baskets](<904. Fruit Into Baskets.md>) | Condition: at most 2 distinct types | 2461 ka `freq + distinct` + 1004 ka maximize template → strings ke freq-map windows ka bridge | `if (bucket[arr[right]]==1) unique_count++;` + `while (unique_count > 2)` |

## Gotchas
- Fixed window mein shrink ke liye `if` kaafi (size ek step mein 1 hi badhta hai). Minimize mein `while` zaroori: 209 pe `[1,1,1,1,100], target=100` → `if` se 5 aata hai, sahi 1.
- Record ki jagah: maximize → shrink loop ke baad (window valid). Minimize → shrink loop ke andar, `left` badhane se pehle.
- Window length: closed `[left, right]` → `right-left+1`. 209 mein `right++` pehle hai → `[left, right)` → `right-left`.
- Freq array ka size value range pe tika hai (2461 mein `100001`, 904 mein `n`) — constraint check karo.
- 713 jaise product windows mein `k <= 1` guard — warna khaali window bhi invalid lagti hai aur `left` `right` se aage nikal jaata hai.

## Related elsewhere
- [Strings sliding window](<../../Strings/Sliding_Window/sliding_window.md>) — next chapter: yahi template strings pe freq map ke saath (3, 424, 567, 76). [904](<904. Fruit Into Baskets.md>) ↔ [GFG K Uniques](<../../Strings/Sliding_Window/Longest Substring with K Uniques.md>) — same "≤K distinct" idea.
- [Prefix Sum](<../Prefix_Sum/prefix_sum.md>) — negative numbers ke saath `sum == k` ([560](<../Prefix_Sum/560.Subarray Sum Equals K.md>)): monotonic nahi → window nahi chalega.
- [Kadane](<../Kadane/kadane.md>) — size ki koi condition nahi, bas max sum subarray (53).
