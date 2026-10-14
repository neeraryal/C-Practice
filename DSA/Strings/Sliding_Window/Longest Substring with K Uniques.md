# GFG – Longest Substring with K Uniques
> Lowercase string mein sabse lambi substring ki length do jisme **exactly** `k` distinct characters hon (na mile to -1)

| Pattern | Data structure | Time | Space |
|---|---|---|---|
| [Sliding window – freq array + distinct count, maximize](sliding_window.md) | `int arr[26]` freq + `count` (distinct) | O(n) | O(1) (26 ints) |

**How it works**
- **Soch:** Har substring check karna O(n²) hai. Window mein kitne distinct chars hain, ye `arr[26]` + `count` se O(1) mein pata chalta hai: freq 0→1 pe `count++`, 1→0 pe `count--`. `count > k` → window invalid → left se shrink.
- **Shrink (pehle):** Har step ki shuruaat mein `while (count > k)`: `s[left]` ka freq ghatao, 0 hua to `count--`, `left++`.
- **Expand + record:** Phir `s[right]` lo (freq 0 tha → `count++`). `count == k` ho tabhi `res = max(res, window)`.
- **Kyun sahi hai:** Shrink `s[right]` add karne se *pehle* hai, to ek step ke liye window mein `k+1` distinct reh jaate hain. Par us step pe `count == k` false hai → wo invalid window record nahi hoti, aur agle step ki shrink use theek kar deti hai. Har char ek baar andar, ek baar bahar → O(n).
- **Dry run:** `"aabacbebebe", k=3`
  - `r=5`: window `aabacb`, count 3 → res 6
  - `r=6`: `e` aaya → count 4 → record nahi
  - `r=7`: pehle shrink `aaba` bahar (left=4), phir `b` → window `cbeb`, count 3
  - `r=10`: window `cbebebe` → res 7 ✔

**Gotcha:** "Exactly k" hai, "at most k" nahi — isliye record sirf `count == k` pe aur `res = -1` se start (k distinct kabhi na bane, jaise `"aaaa", k=2` → -1). `s[x]-'a'` sirf lowercase input pe safe hai.
**vs related:** [904. Fruit Into Baskets](<../../Arrays/Sliding_Window/904. Fruit Into Baskets.md>) yahi "≤ k distinct" idea hai, `k=2` aur int array pe (at most → har valid window record). [424](<424. Longest Repeating Character Replacement.md>) mein constraint distinct count nahi, `window - max_freq ≤ k` hai.

<details><summary>Problem statement</summary>

You are given a string s consisting only lowercase alphabets and an integer k. Your task is to find the length of the longest substring that contains exactly k distinct characters.

Note : If no such substring exists, return -1. 

Examples:

Input: s = "aabacbebebe", k = 3
Output: 7
Explanation: The longest substring with exactly 3 distinct characters is "cbebebe", which includes 'c', 'b', and 'e'.
Input: s = "aaaa", k = 2
Output: -1
Explanation: There's no substring with 2 distinct characters.
Input: s = "aabaaab", k = 2
Output: 7
Explanation: The entire string "aabaaab" has exactly 2 unique characters 'a' and 'b', making it the longest valid substring.

</details>

## Code

```cpp

class Solution {
  public:
    int longestKSubstr(string &s, int k) 
    {
        int arr[26]={};  // freq array: window mein har char kitni baar hai
        int n= s.size();
        int left=0, right=0;
        int res=-1, window=0;  // -1 = exactly k wali window abhi tak nahi mili
        int count =0 ;  // window mein kitne distinct chars hain
        while (right< n)
        {
            
            // shrink PEHLE: pichle step mein s[right] ne count k+1 kiya tha
            while(count>k)
            {
                arr[s[left]-'a'] -=1;  // shrink: s[left] window se bahar
                if(arr[s[left]-'a']==0) count --;  // us char ki last copy gayi → distinct count recalculate
                left++;
            }
            if(arr[s[right]-'a']==0) count ++;  // expand: freq 0 tha → naya char → distinct++ (check ++ se pehle)
            arr[s[right]-'a'] +=1;
            window= right-left+1;
            if(count==k) res= res>window? res : window;  // record sirf count==k pe → k+1 wali invalid window skip
            right++;
        }
        return res;
        
    }
};

```
