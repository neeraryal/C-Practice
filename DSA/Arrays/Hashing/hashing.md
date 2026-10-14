# Hashing Pattern

**Core idea:** Hash set / map se "ye value pehle dekhi hai?" ya "kitni baar dekhi?" O(1) avg mein answer hota hai. Nested loop (O(n²)) ya sort (O(n log n)) ki jagah ek pass + lookup. Sirf presence chahiye to set, count / index chahiye to map. Top k jaisa ranking chahiye to map ke upar heap (ya bucket).

**Signals**
- "Duplicate hai?", "pehle dekha?", "exists?" type checks
- Unsorted array, aur O(n) chahiye (sort mehenga ya allowed nahi)
- Neighbour / partner lookup: `x-1`, `x+1`, `target - x` present hai?
- Frequency based answer: "most frequent", "top k", "kitni baar"

## Template
```cpp
unordered_set<int> seen;              // presence chahiye → set
for (int x : nums) {
    if (seen.count(x)) return true;   // pehle dekha → answer mil gaya
    seen.insert(x);                   // check ke BAAD insert
}
/* count chahiye: unordered_map<int,int> freq; freq[x]++; */
/* top k: freq map → priority_queue<pair<int,int>, vector<...>, greater<...>> (size-k min-heap) */
```

## Progression
| # | Problem | What changes | New intuition | Key code delta |
|---|---|---|---|---|
| 1 | [217. Contains Duplicate](<217. Contains Duplicate.md>) | Base: koi value repeat hui? | Seen-before set: har element pe O(1) "pehle dekha?" check, phir insert | `if(mapp.count(it) > 0) return true;` + `mapp.insert(it);` |
| 2 | [128. Longest Consecutive Sequence](<128. Longest Consecutive Sequence.md>) | Ab `x` ke neighbours (`x+1`) chahiye, longest run | Pehle sab set mein; sirf **start** (`x-1` absent) se walk karo → har run ek baar gina jaata hai | `if(!s.count(it-1))` → `while(s.count(current+1)) count++;` |
| 3 | [347. Top K Frequent Elements](<347. Top K Frequent Elements.md>) | Presence nahi, **count** chahiye + top k | Freq map, phir `{freq, num}` size-k min-heap: size > k pe sabse kam freq pop | `mapp[it]++;` + `maxpq.push({freq,num}); if(maxpq.size()>k) maxpq.pop();` |

## Gotchas
- Check **pehle**, insert **baad mein** (217) — ulta karoge to element khud ko duplicate dikhayega.
- 128: `std::set` tree hai → har lookup O(log n); O(1) avg ke liye `unordered_set`. Aur walk wala loop **set pe** chalao, `nums` pe nahi — warna duplicate starts pe run baar baar walk hota hai (O(n²), measured 93.7 s at n≈1e5).
- 347: top k rakhne ke liye **min**-heap (`greater<>`) chahiye — evict sabse kamzor ko karna hai. Code mein naam `maxpq` misleading hai.
- `unordered_*` sirf average O(1) hai; sorted order chahiye to `set` / `map` (O(log n)).

## Related elsewhere
- [1. Two Sum](<../Two_Sum/1. Two_Sum_Problem.md>) — hash map mein partner `target - x` dhoondho (value → index).
- [560. Subarray Sum Equals K](<../Prefix_Sum/560.Subarray Sum Equals K.md>) — prefix sums ki frequency map: `sum - k` kitni baar dekha.
- Strings/Hashing: [242. Valid Anagram](<../../Strings/Hashing/242. Valid Anagram.md>), [451. Sort Characters By Frequency](<../../Strings/Hashing/451. Sort Characters By Frequency.md>) — chars pe freq map; 451 ka bucket sort 347 ko O(n) bana deta hai.
- [Strings hashing](<../../Strings/Hashing/hashing.md>) — freq signature, anagrams, char mapping (242 → 49 → 451 → 205).
