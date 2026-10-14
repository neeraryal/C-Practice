# Kadane Pattern

**Core idea:** Har index pe ek hi sawaal: purane subarray ko aage badhao ya yahin se naya shuru karo? Running `sum` answer ko bigaadne lage (max ke liye ≤ 0, min ke liye ≥ 0) to use drop karo — wo prefix aage kisi kaam ka nahi. `res` ko `arr[0]` se start karo aur reset se pehle record karo, taaki all-negative / all-positive array bhi sahi chale. Stock problems isi ka roop hain: `sell - buy` = daily price differences ka subarray sum.

**Signals**
- "Contiguous subarray ka max / min sum"
- Ek pass, O(1) space chahiye; brute force saare subarrays O(n²)
- "Buy low, sell high" — prices ke din-ba-din differences

## Template
```c
int sum = 0, res = arr[0];             /* res = arr[0] → all-negative bhi sahi */
for (int i = 0; i < n; i++) {
    sum += arr[i];                     /* current subarray extend */
    res = sum > res ? sum : res;       /* record answer — reset se PEHLE */
    if (sum <= 0) sum = 0;             /* bekaar prefix → agle element se naya start */
}
/* min ke liye: res = min(res, sum), reset if (sum >= 0) */
```

## Progression
| # | Problem | What changes | New intuition | Key code delta |
|---|---|---|---|---|
| 1 | [53. Maximum Subarray](<53. Maximum Subarray(Kadanes).md>) | Base problem: contiguous max sum | Negative running sum aage ke liye bojh hai → reset 0. `res = arr[0]` + reset se pehle record → all-negative safe | `res = res > sum ? res : sum; if (sum <= 0) sum = 0;` |
| 2 | [GFG Smallest sum contiguous subarray](<Smallest sum contiguous subarray(Kadanes).md>) | Max ki jagah min | Mirror image: positive running sum min ko bigaadta hai → usse reset | `res = res < sum ? res : sum; if (sum >= 0) sum = 0;` |
| 3 | [121. Best Time to Buy and Sell Stock](<121. Best Time to Buy and Sell Stock.md>) | Array = prices, ek buy + baad mein ek sell | `sell - buy` = beech ke daily differences ka sum → differences pe Kadane. Seedha form: running min buy price track karo | `if (buy_price > arr[i]) buy_price = arr[i]; else profit = max(profit, arr[i] - buy_price);` |
| 4 | [122. Best Time to Buy and Sell Stock II](<122. Best Time to Buy and Sell Stock II.md>) | Unlimited transactions | Ek best subarray nahi, saare positive differences le sakte ho → greedy, na min na reset | `val = arr[i] - arr[i-1]; profit += val > 0 ? val : 0;` |

## Gotchas
- `res = 0` se start mat karo — `[-3,-1,-2]` pe 53 ka answer `-1` hai, 0 nahi. Isliye `res = arr[0]` aur record **reset se pehle**.
- Max ke liye reset `sum <= 0` pe, min ke liye `sum >= 0` pe — comparison aur reset dono flip karne hain.
- 121 mein `profit = 0` start sahi hai — problem khud kehti hai profit na bane to 0 (trade hi mat karo).
- 121 vs 122: ek transaction → running min; unlimited → har positive diff jodo.
- Subarray ke indices bhi chahiye to reset pe `start = i + 1` aur record pe `best_l = start, best_r = i` rakho (user code mein `left` isi ke liye tha, abhi unused).

## Related elsewhere
- [Prefix Sum](<../Prefix_Sum/prefix_sum.md>) — subarray sum = `prefix[r] - prefix[l-1]`; max subarray = har `r` pe "prefix minus ab tak ka min prefix" — bilkul 121 jaisa.
- [Boyer–Moore Voting](<../Boyer_Moore_Voting/boyer_moore_voting.md>) — same one-pass running state jo 0 pe reset hota hai (wahan sum ki jagah vote count).
- [1D DP](<../../Dynamic_Programming/1D_DP/1d_dp.md>) — Kadane asal mein 1D DP hai: `best_ending_at(i) = max(arr[i], best_ending_at(i-1) + arr[i])`, sirf last state yaad rakho.
