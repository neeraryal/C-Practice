# Linked List Reversal Pattern

**Core idea:** Singly list reverse karne ka matlab hai har node ka `next` pichhle node ki taraf modna. Link todte hi aage ka raasta kho jaata hai → **save, flip, advance** isi order mein. Poori list (206) ho ya chhote groups (24), idea same hai; group wale case mein group ke peeche wala node bhi relink hota hai, isliye `dummy` + `prev` chahiye.

**Signals**
- List / sublist / k-group "reverse" karna, ya pairs swap — values nahi, links badalne hain
- Second half ulta karke compare / merge (palindrome, reorder list)
- O(1) extra space, in-place

## Template
```c
/* Poori list reverse */
Node *prev = NULL, *curr = head;
while (curr) {
    Node *next = curr->next;    /* save: aage ka part */
    curr->next = prev;          /* flip */
    prev = curr;                /* advance */
    curr = next;
}
return prev;                    /* naya head (purana tail) */

/* Group-wise (k = 2): dummy + prev = group se just pehle wala node */
Node *dummy = malloc(sizeof(Node)); dummy->next = head;
Node *p = dummy;
while (p->next && p->next->next) {
    Node *a = p->next, *b = a->next;
    a->next = b->next;          /* baaki list pehle bachao */
    b->next = a;                /* swap */
    p->next = b;                /* pichhla part naye group head se */
    p = a;                      /* a ab group ka last → agla group */
}
Node *res = dummy->next; free(dummy); return res;
```

## Progression
| # | Problem | What changes | New intuition | Key code delta |
|---|---|---|---|---|
| 1 | [206. Reverse Linked List](<206. Reverse Linked List.md>) | Base: poori list ulti | Save → flip → advance; loop ke baad `prev` hi naya head | `next = current->next; current->next = prev; prev = current; current = next;` |
| 2 | [24. Swap Nodes in Pairs](<24. Swap Nodes in Pairs.md>) | Sirf 2-2 ke groups palatne hain | Group ke peeche wala node (`prev`) bhi relink hota hai → head group ke liye `dummy`. Swap ke baad `first` hi group ka last → `prev = first` | `first->next = second->next; second->next = first; prev->next = second; prev = first;` |

## Gotchas
- Flip se pehle `next` save karo — warna baaki list gum.
- 206 mein return `prev`, `current` nahi (loop ke baad wo NULL hai).
- 24 mein `first->next = second->next` sabse pehle; return `dummy->next`, `head` nahi (head ab second position pe hai).
- `malloc` kiya `dummy` kabhi `free` nahi hota (24) → leak; LeetCode pe chalta hai.

## Related elsewhere
- [234. Palindrome Linked List](<../Fast_Slow/234. Palindrome Linked List.md>) — middle dhoondh ke sirf second half pe 206 wala reverse.
- [Fast & Slow](<../Fast_Slow/fast_slow.md>) — middle finder aur dummy-node trick (876, 19).
