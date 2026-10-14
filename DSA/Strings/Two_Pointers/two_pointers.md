# Two Pointers Pattern (Strings)

**Core idea:** String pe do indices chalao — cleaned copy, stack ya extra buffer ke bina. Teen shapes: **dono ends se andar** (125: faltu chars skip karke compare), **right se left** (844, 151: jab order/effect right se samajh aata hai), aur **center se bahar** (5: palindrome expand). Reverse scan (844): Do strings ko "process" karne ke baad compare karna hai (yahan `#` = backspace). Naya string banane ki jagah har string pe ek pointer chalao aur on-the-fly agla valid char nikaalo. Backspace apne *left* ko affect karta hai, isliye **right se left** scan — `#` pehle milta hai aur skip count pata rehta hai. O(1) extra space.

**Signals**
- Do strings / sequences ko processing ke baad compare karna (backspace, ignore chars)
- O(1) space chahiye (stack ya naya string nahi)
- Koi operator apne pichhle char ko affect karta hai → reverse scan
- "Palindrome hai? (symbols/case ignore)" → dono ends se andar, faltu chars skip (125)
- "Words ka order ulta / extra spaces hatao" → right se scan, `left`/`right` = word boundaries (151)
- "Longest palindromic substring" → har center se bahar expand, odd + even centers (5)

## Template
```c
int next_valid(char *s, int i) {            /* i se left mein agla bachne wala char */
    int skip = 0;
    for (; i >= 0; i--) {
        if (s[i] == '#') skip++;            /* backspace pending */
        else if (skip > 0) skip--;          /* ye char mit gaya */
        else return i;                      /* bach gaya */
    }
    return -1;                              /* kuch nahi bacha */
}
/* compare loop */
while (i >= 0 || j >= 0) {
    i = next_valid(s, i); j = next_valid(t, j);
    if (i < 0 || j < 0) return i == j;      /* dono khatam → true, ek hi → false */
    if (s[i] != t[j]) return false;
    i--; j--;                               /* matched char ke ek peeche */
}
return true;
/* 125: dono ends se andar, faltu chars skip */
while (l < r) {
    while (l < r && !isalnum(s[l])) l++;
    while (l < r && !isalnum(s[r])) r--;
    if (tolower(s[l++]) != tolower(s[r--])) return false;
}
/* 5: center (a, b) se bahar expand → length b - a - 1 */
while (a >= 0 && b < n && s[a] == s[b]) { a--; b++; }
```

## Progression
| # | Problem | What changes | New intuition | Key code delta |
|---|---|---|---|---|
| 1 | [125. Valid Palindrome](<125. Valid Palindrome.md>) | Base: ek string, dono ends se compare | Cleaned string banane ki jagah pointers hi faltu chars skip karein; skip loops mein bhi `left<right` | `while(left<right && !check_is_alnum(s[left])) left++;` |
| 2 | [844. Backspace String Compare](<844. Backspace String Compare.md>) | Ab do strings, aur `#` apne pichhle char ko mitata hai | `#` left ko mitata hai → right se scan + skip count, final string banaye bina compare | `s_i = i - 1; t_i = j - 1;` (match ke baad ek peeche) |
| 3 | [151. Reverse Words in a String](<151. Reverse Words in a String.md>) | Compare nahi, naya string banana: words ulte order mein, extra spaces gayab | 844 ki tarah right se scan, par ek hi string pe do pointers: `right` = word end, `left` = word start dhoondhta hai; boundary = `s[left-1]==' '` | `if(left>0 && s[left-1]==' ')` → `res.append(s,left,right-left+1);` |
| 4 | [5. Longest Palindromic Substring](<5. Longest Palindromic Substring.md>) | Poori string check nahi, sabse lamba palindromic substring dhoondhna | 125 ka ulta: pointers center se **bahar** expand; 2n−1 centers (odd `(i,i)`, even `(i,i+1)`) | `while(a>=0 && b<s.size() && s[a]==s[b]) {a--; b++;}` → `return b-a-1;` |

## Gotchas
- Match ke baad pointer `i-1` pe le jao — `i` pe chhoda (ya `s_i=i--` jaisa post-decrement) to same char dobara milega → 844 ka ⚠ bug yahi hai.
- Loop condition `||` hai, `&&` nahi — ek string khatam hone ke baad doosri ke bache chars bhi check hone chahiye (`"a"` vs `"ba"`).
- Empty text pe backspace = no-op → skip count bas bacha rehta hai, index `-1` aata hai.
- Skip loops ke andar bhi `left<right` guard (125) — `".,"` jaise all-symbol input pe warna pointers cross karke aage nikal jaate.
- Expand ke baad `a`, `b` ek-ek step bahar hain → length `b-a-1`; even center pe `start = i-(len-1)/2` integer division se sahi aata hai (5).
- Words ke beech separator sirf `!res.empty()` pe (151) → leading/trailing/multiple spaces output mein nahi aate.

## Related elsewhere
- [Arrays two pointers](<../../Arrays/Two_Pointers/two_pointers.md>) — same-direction read/write pointers; 844 ko us style mein in-place bhi kar sakte ho (`#` pe write pointer peeche).
- [234. Palindrome Linked List](<../../Linked_List/Fast_Slow/234. Palindrome Linked List.md>) — list pe ends se andar nahi chal sakte → middle dhoondh ke second half reverse, phir compare.
