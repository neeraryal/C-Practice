# Monotonic Stack Pattern

**Core idea:** Jin elements ka answer ("next greater") abhi nahi mila, unke **indices** stack mein wait karte hain. Stack monotonic decreasing rehta hai (bottom → top values ghatti hui). Naya element aaye to **jab tak current top se bada hai, pop karo — current hi un sabka answer hai**, phir current ko push. Har index ek baar push, ek baar pop → O(n), brute force O(n²) ki jagah.

**Signals**
- "Next / previous greater / smaller element"
- Har element ke liye "pehla element jo condition satisfy kare" — left ya right side
- Daily temperatures, stock span, histogram largest rectangle type problems

## Template
```c
for (int i = 0; i < n; i++) ans[i] = -1;          /* default: koi bada nahi mila */
for (int i = 0; i < n; i++) {
    while (!isEmpty() && arr[i] > arr[peek()])    /* current bada → top ka answer mil gaya */
        ans[pop()] = arr[i];
    push(i);                                      /* i ab apne answer ka wait karega */
}
/* next smaller: '>' ko '<' karo; previous greater: right se left scan */
```

## Progression
| # | Problem | What changes | New intuition | Key code delta |
|---|---|---|---|---|
| 1 | [Next Greater Element](<Next_greater.md>) | Base problem | Pending indices ka decreasing stack; bada element aate hi pop → wahi answer | `while (top != NULL && arr[i] > arr[peek()]) ans[pop()] = arr[i]; push(i);` |

## Gotchas
- Stack mein **indices** rakho, values nahi — answer `ans[index]` mein likhna hai (distance chahiye to bhi index kaam aata hai).
- `>` strict → equal element "greater" nahi; `>=` karoge to equal pe bhi pop hoga (next greater-or-equal).
- Loop ke baad stack mein bache indices ka answer `-1` — ya `ans` ko pehle se `-1` se init kar do.
- Empty check `&&` mein pehle — empty stack pe `peek()` `-1` deta hai, `arr[-1]` UB hota.

## Related elsewhere
- Stack yahan linked list se bana hai (`push` / `pop` / `peek` with `malloc` / `free`) — repo root ke `Stack_in_c.c` wala implementation (DSA/ ke bahar). Array stack (`int st[n]; int top = -1;`) se bhi same algorithm chalta hai, kam overhead ke saath.
