# Boyer–Moore Voting Pattern

**Core idea:** Do *alag* elements ka pair ek-dusre ko cancel kar do. Majority element `n/2` se zyada baar hai, to baaki saare elements mil ke bhi use poora cancel nahi kar sakte — last mein candidate wahi bachta hai. Bas ek `candidate` aur ek `count` chahiye: O(n) time, O(1) space. Majority guaranteed ho to answer seedha candidate hai, warna ek verify pass.

**Signals**
- "Element jo `> n/2` (ya `> n/3`) baar aata hai"
- O(1) extra space chahiye (hash map count nahi chalega)
- Majority guaranteed hai, ya ek extra verify pass allowed hai

## Template
```c
int cand = 0, count = 0;
for (int i = 0; i < n; i++) {
    if (count == 0) cand = arr[i];     /* sab cancel ho gaye → naya candidate */
    if (arr[i] == cand) count++;       /* support vote */
    else                count--;       /* alag element → ek pair cancel */
}
/* guarantee nahi hai to: doosre pass mein cand ka count > n/2 check karo */
return cand;
```

## Progression
| # | Problem | What changes | New intuition | Key code delta |
|---|---|---|---|---|
| 1 | [169. Majority Element](<169. Majority Element.md>) | Base problem | Alag elements ke pairs cancel → majority survive karta hai; guarantee hai to verify pass nahi | `if (count == 0) cand = arr[i]; count += (arr[i] == cand) ? 1 : -1;` |

## Gotchas
- Candidate sirf "agar majority hai to yahi hai" batata hai. Guarantee na ho to doosra pass zaroori — `[1,2,3]` pe bhi candidate `3` aa jaata hai.
- Final `count` majority ki asli frequency nahi hai, sirf bacha hua surplus hai.
- `> n/3` variant: 2 candidates + 2 counts, aur wahan verify pass hamesha chahiye.

## Related elsewhere
- [Kadane](<../Kadane/kadane.md>) — same one-pass running state jo 0 hone pe reset hota hai (wahan sum, yahan vote count).
