# Hashing Pattern (Strings)

**Core idea:** String ka ek "signature" banao jo chars ka order ignore kare par content pakde — har char ka **frequency count**. Alphabet chhota hai (26 / 128 / 256) to hash map ki jagah seedha array. Phir signature ko compare karo (242), map ka **key** banao (49), count ko **index** banao (451), ya count ki jagah **char → char mapping** rakho (205).

**Signals**
- "Anagram / rearrange / permutation" → freq signature same hai?
- "Group karo jo same letters se bane" → signature = hash map key
- "Frequency ke order mein sort karo" → count map + bucket by count
- "Mapping consistent hai?" (isomorphic, word pattern) → do-taraf map

## Template
```cpp
int freq[26] = {0};
for (char c : s) freq[c - 'a']++;              /* count */
for (char c : t) freq[c - 'a']--;              /* cancel → sab 0 = anagram (242) */

string key;                                    /* 49: counts → key, separator zaroori */
for (int i = 0; i < 26; i++) key += to_string(cnt[i]) + '#';
groups[key].push_back(word);

vector<vector<char>> bucket(n + 1);            /* 451: count hi index (max count n) */
for (auto [ch, f] : mp) bucket[f].push_back(ch);
for (int f = n; f >= 1; f--) for (char ch : bucket[f]) res.append(f, ch);
```

## Progression
| # | Problem | What changes | New intuition | Key code delta |
|---|---|---|---|---|
| 1 | [242. Valid Anagram](<242. Valid Anagram.md>) | Base: do strings, same letters same counts? | `freq[26]`: `s` se `++`, `t` se `--` → sab 0 = anagram. Sort (n log n) ki zarurat nahi | `freq[s[i]-'a']++; freq[t[i]-'a']--;` |
| 2 | [49. Group Anagrams](<49. Group Anagrams.md>) | Do nahi, n strings ko groups mein baantna | Freq signature hi hash map ka **key** → same key = same group. Counts ke beech `#` taaki `1,11` aur `11,1` alag | `key+=to_string(str_freq[i]); key+='#';` + `mapp[key].push_back(it);` |
| 3 | [451. Sort Characters By Frequency](<451. Sort Characters By Frequency.md>) | Count se compare nahi, output ko count ke order mein banana | Count `1..n` range mein hai → count ko index banao (bucket sort) → O(n), `sort` nahi | `bucket[fr].push_back(ch);` + `for (i = n; i >= 1; i--) res.append(i, ch);` |
| 4 | [205. Isomorphic Strings](<205. Isomorphic Strings.md>) | Counts nahi, position-wise char → char mapping | Map ki value ab count nahi, mapped char. Bijection ke liye **dono directions** ka map, `0` = unmapped | `if(s_in_t[t[i]] != s[i]) return false;` + `s_in_t[t[i]] = s[i]; t_in_s[s[i]] = t[i];` |

## Gotchas
- `c - 'a'` sirf lowercase pe; uppercase/digits ho (451) to `freq[128]` ya `unordered_map`.
- 49 ke key mein separator ke bina `a=1,b=11` aur `a=11,b=1` dono `"111..."` → galat grouping.
- `char` ko array index banana: signed hai → non-ASCII pe negative index (205 constraints ki wajah se safe).
- One-way map isomorphic ke liye kaafi nahi: `s="ab", t="aa"` → reverse map hi pakadta hai.
- `unordered_map` ka iteration order fixed nahi → 49/451 ka output order bhi nahi (dono "any order" allow karte hain).

## Related elsewhere
- [567. Permutation in String](<../Sliding_Window/567. Permutation in String.md>) — 242 ka freq signature ek fixed sliding window mein: expand pe `--`, shrink pe `++`.
- [Strings sliding window](<../Sliding_Window/sliding_window.md>) — window state = `freq[26]` + ek counter.
- [347. Top K Frequent Elements](<../../Arrays/Hashing/347. Top K Frequent Elements.md>) — 451 jaisa "count → frequency order", numbers pe (heap / bucket).
- [217. Contains Duplicate](<../../Arrays/Hashing/217. Contains Duplicate.md>) — sabse basic hashing: "ye pehle dekha hai?" set se.
- [Arrays hashing](<../../Arrays/Hashing/hashing.md>) — same set/map idea numbers pe (217 → 128 → 347).
