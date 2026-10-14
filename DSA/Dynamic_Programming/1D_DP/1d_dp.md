# 1D DP Pattern

**Core idea:** Answer `F(n)` chhote overlapping subproblems se banta hai. Teen steps: (1) **plain recursion** — `fib(n-1) + fib(n-2)` same values baar-baar compute karta hai → O(2^n); (2) **memo** (top-down) — har `F(i)` ek baar compute karke array mein save → O(n) time, O(n) space; (3) **bottom-up** — `F(0)` se upar banao, aur `F(i)` sirf last 2 pe depend hai to array bhi nahi, do rolling variables kaafi → O(n) time, O(1) space. User ka 509 code step (3) hai.

**Signals**
- "n-th term / number of ways / min cost to reach i" jahan `F(i)` pichle kuch states se banta hai
- Recursion tree mein same subproblem baar-baar aata hai
- Recurrence sirf last `k` states pe depend → O(1) space rolling variables

## Template
```c
/* recurrence: dp[i] = dp[i-1] + dp[i-2] */
if (n < 2) return n;                  /* base cases */
int a = 0, b = 1;                     /* a = dp[i-2], b = dp[i-1] */
for (int i = 2; i <= n; i++) {
    int c = a + b;                    /* dp[i] */
    a = b;                            /* window slide */
    b = c;
}
return b;                             /* dp[n] */
```

## Progression
| # | Problem | What changes | New intuition | Key code delta |
|---|---|---|---|---|
| 1 | [509. Fibonacci Number](<509. Fibonacci Number.md>) | Base problem | `F(i)` sirf `F(i-1)`, `F(i-2)` pe depend → dp array ki jagah 2 rolling variables | `c = a + b; a = b; b = c;` |

## Gotchas
- Base cases pehle handle karo — 509 mein `n=0` pe loop nahi chalta aur `b=1` galat return hota.
- Rolling update ka order: pehle `c = a + b`, phir `a = b`, phir `b = c` — ulta kiya to value overwrite ho jaati hai.
- `int` overflow: `F(47)` `int` se bahar — bade `n` ke liye `long long` ya modulo.
- Memo (top-down) mein recursion depth O(n) → bahut bade `n` pe stack overflow; bottom-up safe hai.

## Related elsewhere
- [Kadane](<../../Arrays/Kadane/kadane.md>) — O(1)-space 1D DP: `best_ending_at(i) = max(arr[i], best_ending_at(i-1) + arr[i])`, sirf last state yaad.
