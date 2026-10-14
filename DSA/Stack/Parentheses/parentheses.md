# Parentheses Pattern

**Core idea:** Har `(` stack pe push, har `)` top ko pop karta hai. Agar sirf ek bracket type ho aur input valid ho, to stack ke andar kya hai matter nahi karta — sirf uska size → ek `open_count` counter hi "implicit stack" hai (O(1) space). Multiple types aate hi yaad rakhna padta hai **kaunsa** bracket last khula tha → real stack + closing→opening map.

**Signals**
- "Valid / balanced parentheses", "nesting depth", "primitive / outermost"
- Sirf `(` `)` aur valid input → counter; `()[]{}` mix ya invalid ho sakta hai → stack
- Last opened, first closed (LIFO) matching

## Template
```cpp
int open_count = 0;                     // implicit stack size = current depth
for (char c : s) {
    if (c == '(') open_count++;         // push
    else if (c == ')') open_count--;    // pop
    /* depth = open_count; open_count == 0 → ek balanced block poora hua */
}
/* multiple types: stack<char> st; opening → push;
   closing → st.empty() ya st.top() != match[c] → false, warna pop; end mein return st.empty(); */
```

## Progression
| # | Problem | What changes | New intuition | Key code delta |
|---|---|---|---|---|
| 1 | [1614. Maximum Nesting Depth of the Parentheses](<1614. Maximum Nesting Depth of the Parentheses.md>) | Base: valid string, ek type, max depth | Stack ka size hi depth hai → counter kaafi, max over time lo | `res=max(res,open_count);` |
| 2 | [1021. Remove Outermost Parentheses](<1021. Remove Outermost Parentheses.md>) | Depth nahi, primitive boundaries chahiye | Counter wapas 0 → ek primitive `s[left..right]` khatam → andar ka part append | `if(open_count ==0) { res.append(s,left+1, right-left-1); left=right+1; }` |
| 3 | [20. Valid Parentheses](<20. Valid Parentheses.md>) | 3 bracket types + input invalid ho sakta hai | Counter `([)]` ko valid bolega → real stack + closing→opening map | `st.empty()` ya `st.top() != mapp[c]` → `return false;` warna `st.pop();` |

## Gotchas
- Counter trick tabhi chalti hai jab ek type ho **aur** input valid ho (1614, 1021 mein guaranteed). Invalid ho sakta hai to `open_count < 0` bhi check karna padega.
- 20: `st.empty()` check `top()` se **pehle** — empty stack pe `top()` UB (`")"`).
- 20: Loop ke baad `st.empty()` return karo — `"(("` jaise unclosed openings warna valid dikhenge.
- 1021: `string::append(s, pos, len)` ka 3rd arg **length** hai, end index nahi → `right-left-1`.

## Related elsewhere
- [Monotonic Stack](<../Monotonic_Stack/monotonic_stack.md>) — stack ka doosra bada use: pending elements ka "next greater" answer.
