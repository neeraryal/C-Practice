# Sliding Window (Strings) Pattern

**Core idea:** Ye [Arrays sliding window](<../../Arrays/Sliding_Window/sliding_window.md>) ke upar hi bana hai — same expand/shrink template. Farak sirf window state ka hai: sum ki jagah ek **frequency array** (`freq[26]` letters ke liye, `freq[128]` poore ASCII ke liye) plus ek **counter** (`count` distinct / `matched` / `max_freq`) jo O(1) mein batata hai ki window valid hai ya nahi. Expand pe `freq[c]++` + counter recalculate, shrink pe `freq[c]--` + counter recalculate.

**Signals**
- "Longest / shortest **substring** jisme ..." — k distinct chars, koi repeat nahi, at most k changes
- "Permutation / anagram of `s1` in `s2`" → fixed window = `len(s1)`, freq signature match
- "Window jisme `t` ke saare chars hon" → `matched` / `required` counter
- Alphabet chhota hai (26 / 128) → hash map nahi, seedha freq array

## Template
```c
int freq[26] = {0};
int left = 0, count = 0, res = 0;
for (int right = 0; s[right] != '\0'; right++) {
    if (freq[s[right] - 'a']++ == 0) count++;     /* expand + recalculate (yahan: distinct) */
    while (count > k) {                            /* window invalid */
        if (--freq[s[left] - 'a'] == 0) count--;   /* shrink + recalculate */
        left++;
    }
    if (right - left + 1 > res) res = right - left + 1;   /* record answer (maximize) */
}
/* Minimize (76): while (window valid) { record; shrink; }  → record shrink loop ke ANDAR */
/* Fixed (567): index >= len ho to s[index - len] bahar → har step ek andar, ek bahar */
```

## Progression
| # | Problem | What changes | New intuition | Key code delta |
|---|---|---|---|---|
| 1 | [GFG Longest Substring with K Uniques](<Longest Substring with K Uniques.md>) | Array ki jagah string; window ka constraint = exactly `k` distinct chars, maximize | `arr[26]` + `count`: freq 0→1 pe `count++`, 1→0 pe `count--`. `count > k` → shrink; record sirf `count == k` pe | `if(arr[s[right]-'a']==0) count ++;` + `if(count==k) res= ...` |
| 2 | [3. Longest Substring Without Repeating Characters](<3.Longest Substring Without Repeating Characters.md>) (not solved yet) | Hint: "koi repeat nahi" = window mein har char ka `freq ≤ 1` — naye char ka freq 2 hua to shrink | — | — |
| 3 | [424. Longest Repeating Character Replacement](<424. Longest Repeating Character Replacement.md>) | Constraint distinct count nahi, "at most k chars badal sakte ho" | Replacements = `window_size - max_freq`; `> k` → shrink. `max_freq` shrink pe kabhi kam nahi karte — stale value pe window bas slide karti hai, `res` galat nahi hota | `while((window_size-max_freq_in_window)>k)` |
| 4 | [567. Permutation in String](<567. Permutation in String.md>) | Maximize nahi, yes/no; window size fixed = `len(s1)` | Permutation = same freq signature. `s1` counts `+`, window chars `-` → sab 0 = match. (Approach 2/3: `matched` counter → 26-scan bhi nahi) | `count_arr[s2[index]-'a']--;` + `count_arr[s2[index-character_count]-'a']++;` |
| 5 | [76. Minimum Window Substring](<76. Minimum Window Substring.md>) | Size variable, extra chars allowed, **minimize** | `matched` = window mein `t` ke useful chars; `matched == required` → valid. Valid rehte tak shrink, record shrink loop ke andar | `while(matched == required) { record; s[left] nikalo; left++; }` |

## Gotchas
- Index math alphabet pe depend karta hai: lowercase `- 'a'`, uppercase (424) `- 'A'`, mixed case (76) → `freq[128]` + `(unsigned char)` index.
- Distinct count ke liye `freq == 0` check **increment se pehle** (expand) aur **decrement ke baad** (shrink) — order ulta to count galat.
- Maximize (K Uniques, 424) → record shrink loop ke **baad**; minimize (76) → record shrink loop ke **andar**, har valid window pe.
- "Exactly k" (K Uniques) mein record sirf `count == k` pe aur default `-1`; "at most k" (904) mein har valid window record.
- `calloc` kiya hai to har return path pe `free` — 567 approach 1 `return true` pe leak karta hai.

## Related elsewhere
- [Arrays Sliding Window](<../../Arrays/Sliding_Window/sliding_window.md>) — base template (sum / zero-count wali window).
- [904. Fruit Into Baskets](<../../Arrays/Sliding_Window/904. Fruit Into Baskets.md>) — K Uniques jaisa "≤ 2 distinct", int array pe `bucket[]` freq ke saath.
- [1004. Max Consecutive Ones III](<../../Arrays/Sliding_Window/1004. Max Consecutive Ones III.md>) — 424 ka binary version: `zeros ≤ k`.
- [2461. Maximum Sum of Distinct Subarrays With Length K](<../../Arrays/Sliding_Window/2461. Maximum Sum of Distinct Subarrays With Length K.md>) — 567 jaisi fixed window + freq/distinct check, int array pe.
- [209. Minimum Size Subarray Sum](<../../Arrays/Sliding_Window/209. Minimum Size Subarray Sum.md>) — 76 wala "shrink while valid, minimize" template, sum pe.
