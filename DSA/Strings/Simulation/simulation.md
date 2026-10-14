# Simulation Pattern (Strings)

**Core idea:** String ko waise hi padho jaise problem bolti hai — koi data structure nahi, bas sahi direction aur sahi stop condition. Teen chhote tricks: answer end pe depend kare to **right se scan** (1903: odd-ness = last digit), number banana ho to **left se parse + guard** (8: overflow multiply se pehle pakdo), aur kabhi ek **identity** poora simulation hata deti hai (796: saari rotations `s+s` mein).

**Signals**
- "String mein bada number hai" → integer mein convert mat karo; last digit / prefix pe socho
- "Implement atoi / parse karo" → steps given hain, order mein follow + range clamp
- "Shift / rotate karke ban sakta hai?" → `s+s` (ya `goal+goal`) mein substring search
- Output input ka prefix/substring hai → index dhoondho, phir ek `append`/`substr`

## Template
```cpp
/* Parse with overflow guard (8) */
int i = 0, sign = 1, res = 0;
while (s[i] == ' ') i++;                         /* C++11: s[size()] == '\0' → safe stop */
if (s[i] == '+' || s[i] == '-') sign = (s[i++] == '-') ? -1 : 1;
while (s[i] >= '0' && s[i] <= '9') {
    int d = s[i++] - '0';
    if (res > INT_MAX / 10 || (res == INT_MAX / 10 && d > 7))
        return sign == 1 ? INT_MAX : INT_MIN;    /* multiply se PEHLE check */
    res = res * 10 + d;
}
return res * sign;

/* Right se scan (1903): pehla odd digit → us tak ka prefix */
for (int r = n - 1; r >= 0; r--) if ((num[r] - '0') % 2) return num.substr(0, r + 1);

/* Rotation (796) */
return s.size() == goal.size() && (goal + goal).find(s) != string::npos;
```

## Progression
| # | Problem | What changes | New intuition | Key code delta |
|---|---|---|---|---|
| 1 | [1903. Largest Odd Number in String](<1903. Largest Odd Number in String.md>) | Base: string ek bada number hai (10⁵ digits) | Odd-ness sirf last digit se → right se pehla odd digit, us tak ka prefix = answer. Number banana hi nahi | `if ((num[right] - '0') % 2 == 0) right --;` else `res.append(num,0,right+1);` |
| 2 | [8. String to Integer (atoi)](<8. String to Integer (atoi).md>) | Ab sach mein number banana hai — left se, spaces/sign/garbage ke saath, range ke andar | Steps order mein follow; overflow `res*10` ke **pehle** `INT_MAX/10` aur last digit `7` se check | `res>INT_MAX/10` ya `(res== INT_MAX/10 && temp>7)` → `return sign==1?INT_MAX:INT_MIN;` |
| 3 | [796. Rotate String](<796. Rotate String.md>) | Number nahi, do strings: "shifts se ban sakta hai?" | Har shift simulate karne ki jagah identity: `goal+goal` ke length-n substrings = `goal` ki saari rotations → ek `find` | `string temp= goal+goal; return temp.find(s) != string::npos;` |

## Gotchas
- 10⁵ digit ka "number" `int`/`long` mein nahi aata — string pe hi kaam karo (1903).
- Overflow check multiply ke **baad** bekaar hai — signed overflow ho chuka (UB). Guard hamesha pehle (8).
- `-2147483648` pe guard `INT_MIN` deta hai — clamp aur exact value same hai, isliye `digit > 7` dono signs ke liye chalta hai.
- `std::string` mein `s[s.size()]` `'\0'` hai (C++11) → scan loops bina bounds check ke ruk jaate hain; usse aage padhna UB.
- 796 mein length check zaroori — chhoti string `s+s` mein hamesha mil jaati hai.

## Related elsewhere
- [Arrays simulation](<../../Arrays/Simulation/simulation.md>) — 66 Plus One: digit array pe right se carry; 1903 ki tarah LSD side se kaam.
- [Strings two pointers](<../Two_Pointers/two_pointers.md>) — 151 bhi right se scan karke `append` se answer banata hai.
