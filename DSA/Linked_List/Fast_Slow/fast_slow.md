# Fast & Slow Pointers Pattern

**Core idea:** Linked list mein peeche nahi ja sakte aur length pata nahi. Do pointers ek hi list pe alag **speed** (fast 2x) ya alag **start** (fixed gap) se chalao — unki relative position hi answer de deti hai: fast end pe → slow middle pe; cycle mein fast slow ko pakad leta hai; gap n rakho → fast end pe tab slow end se n peeche. Sab ek pass, O(1) space.

**Signals**
- Linked list ka middle / end se k-th node chahiye, bina length nikale (one pass)
- Cycle detect karna ya cycle ka start dhoondhna, O(1) space mein
- List pe palindrome / reorder / "second half" — pehle middle chahiye

## Template
```c
Node *slow = head, *fast = head;
while (fast && fast->next) {        /* fast end tak → list khatam */
    slow = slow->next;              /* 1 step */
    fast = fast->next->next;        /* 2 step */
    if (slow == fast) { /* cycle */ }
}
/* no cycle: loop ke baad slow = middle (even length mein second middle) */

/* Fixed gap: dummy se start, fast ko n aage, phir dono 1-1 step */
for (int i = 0; i < n; i++) fast = fast->next;
while (fast->next) { fast = fast->next; slow = slow->next; }
/* slow = end se n-th node ka predecessor */
```

## Progression
| # | Problem | What changes | New intuition | Key code delta |
|---|---|---|---|---|
| 1 | [876. Middle of the Linked List](<876. Middle of the Linked List.md>) | Base: middle node | Fast 2x chale to fast ke end tak slow aadha hi chalta hai → middle | `while (f && f->next) { s = s->next; f = f->next->next; }` |
| 2 | [141. Linked List Cycle](<141. Linked List Cycle.md>) | List mein cycle ho sakta hai | Cycle mein fast har step 1 node paas aata hai → milna pakka. Mile → cycle, fast NULL → nahi | `if (slow == fast) return true;` (move ke baad) |
| 3 | [142. Linked List Cycle II](<142. Linked List Cycle II.md>) | Cycle ka start node chahiye | `2(a+b) = a+b+kL` → `a = (k-1)L + c` → meet ke baad slow head pe, dono 1-1 step → entry pe milte hain | `slow = head; while (slow != fast) { slow = slow->next; fast = fast->next; }` |
| 4 | [19. Remove Nth Node From End](<19. Remove Nth Node From End of List.md>) | End se n-th node hatana | Speed nahi, **gap** fix: fast n aage, phir same speed → fast last pe tab slow target se ek pehle. Dummy se head delete bhi normal case | `for (i = 0; i < n; i++) f = f->next;` + `s->next = s->next->next;` |
| 5 | [234. Palindrome Linked List](<234. Palindrome Linked List.md>) | Poori list ko mirror compare karna | 876 se middle, phir second half [reverse](<../Reversal/reversal.md>) → dono halves aage-aage compare | `curr = slow;` reverse loop, phir `while (right_head) if (right_head->val != left_head->val) return false;` |

## Gotchas
- Loop condition `fast && fast->next` — dono checks chahiye, warna `fast->next->next` pe NULL deref.
- Cycle mein **nodes (addresses)** compare karo, values nahi. Check move ke **baad**, warna start mein hi `head == head`.
- 142 phase 2 mein fast bhi sirf **1 step** chalta hai.
- Head delete ho sakta hai (19) → dummy node. Par `malloc` kiya dummy aur hataya node `free` nahi hote → leak.
- 234 list ko modified chhod deta hai (second half reversed) — list wapas chahiye to compare ke baad dobara reverse.

## Related elsewhere
- [Reversal](<../Reversal/reversal.md>) — 234 ka step 2 (206 wala 3-pointer reverse); dummy trick 24 mein bhi.
- [Linked list two pointers](<../Two_Pointers/two_pointers.md>) — same speed, read/write ya do lists pe pointers (83, 21).
- [125. Valid Palindrome](<../../Strings/Two_Pointers/125. Valid Palindrome.md>) — string mein dono ends se seedha compare; list mein ye nahi ho sakta, isliye 234 wala trick.
