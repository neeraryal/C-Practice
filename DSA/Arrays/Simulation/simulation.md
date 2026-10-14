# Simulation Pattern

**Core idea:** Koi clever trick nahi — process ko waise hi code karo jaise haath se karte. Plus One = grade-school addition: rightmost digit se `carry` le ke left chalo. Asli kaam edge cases ka hai: carry last tak bach jaaye (`999 → 1000`), output ka size badal jaaye, aur memory sahi tarah allocate / return ho.

**Signals**
- "Number digits array mein hai" — increment / add / multiply
- Steps clearly defined hain, bas follow karne hain (carry, rotation, spiral walk)
- Output ka size input se alag ho sakta hai

## Template
```c
int carry = 1;                                /* "+1" hi pehla carry hai */
int *res = malloc(sizeof(int) * (n + 1));     /* worst case ek digit extra */
for (int i = n - 1; i >= 0; i--) {            /* LSD se MSD tak */
    int sum = arr[i] + carry;
    res[i + 1] = sum % 10;
    carry = sum / 10;
}
if (carry) { res[0] = 1; *returnSize = n + 1; return res; }
memmove(res, res + 1, n * sizeof(int));       /* pointer shift nahi → block ka start hi return */
*returnSize = n;
return res;
```

## Progression
| # | Problem | What changes | New intuition | Key code delta |
|---|---|---|---|---|
| 1 | [66. Plus One](<66. PlusOne.md>) | Base problem | Carry right se left; `n+1` jagah pehle se, `res[0]` sirf final carry ke liye | `res[i+1] = sum % 10; carry = sum / 10;` |

## Gotchas
- `malloc` block ke beech ka pointer (`res+1`) kabhi return mat karo — caller ka `free()` UB / crash (66 ka ⚠ yahi hai).
- Final carry sirf tab bachta hai jab saare digits 9 hon — `[9,9] → [1,0,0]`.
- Carry 0 hone ke baad baaki digits bas copy hote hain — logic wahi, sirf copy chal raha hai.

## Related elsewhere
- [15. 3Sum](<../Two_Sum/15. 3Sum.md>) — LeetCode C ka "malloc karke return, caller free karega" convention; size pata nahi to `realloc` se grow.
