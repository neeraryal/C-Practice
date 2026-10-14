# Two Pointers Pattern (Linked List)

**Core idea:** Array wale read/write two pointers, bas index ki jagah node pointers. `j` (read) har node pe chalta hai, `i` (write) last "rakhe gaye" node pe rukta hai. Array mein value copy karte the, list mein `i->next` re-link karte hain — koi shifting nahi. Sorted list mein duplicates hamesha consecutive hain, isliye ek hi pass kaafi. Do sorted lists merge karni hon (21) to har list pe ek pointer — chhota head aage, aur nodes re-link hote hain, copy nahi.

**Signals**
- Linked list se kuch nodes in-place hatane hain (duplicates, value match)
- List sorted hai → same values paas-paas
- O(1) extra space, ek pass
- Do sorted lists ko ek sorted list mein merge / splice karna

## Template
```c
if (!head) return head;
Node *i = head, *j = head->next;      /* i = last kept node, j = scan */
while (j) {
    if (i->val == j->val) {           /* hatana hai */
        i->next = j->next;            /* unlink, i wahi rukta hai */
        Node *dup = j; j = j->next; free(dup);   /* aage badhne se pehle free */
    } else {                          /* rakhna hai */
        i = i->next;                  /* i->next == j tha → i = j */
        j = j->next;
    }
}
return head;
```

## Progression
| # | Problem | What changes | New intuition | Key code delta |
|---|---|---|---|---|
| 1 | [83. Remove Duplicates from Sorted List](<83. Remove Duplicates from Sorted List.md>) | Base problem | Sorted → dup hamesha `i` ke just baad; `i->next` re-link se bypass, `i` sirf nayi value pe aage | `if (i->val == j->val) i->next = j->next; else i = i->next;` |
| 2 | [21. Merge Two Sorted Lists](<21. Merge Two Sorted Lists.md>) | Ek ki jagah do sorted lists, merge karni hain | Har list pe ek pointer. Splice version: `l1` = chhote head wali list, uske `<= l2->val` wale poore run ko as-is rakho, run ke end pe `l2` jodo, phir `l1`/`l2` swap. (Common version: `dummy` + `tail`, node-by-node) | `while (l1 && l1->val <= l2->val) { temp = l1; l1 = l1->next; } temp->next = l2;` + swap |

## Gotchas
- `head == NULL` check pehle — warna `head->next` NULL deref.
- Unlink kiye nodes `free` nahi hote to leak (83 mein yahi hai, LeetCode pe chalta hai). Free karna ho to `j` ko temp mein rakh ke pehle aage badhao.
- Invariant `i->next == j` hamesha — isliye `i = i->next` aur `i = j` same hain.
- Head khud kabhi delete nahi hota (pehli value hamesha unique rakhi jaati hai) → dummy node ki zarurat nahi.
- 21 splice merge mein inner loop `<=` hi chahiye — `<` pe equal heads se run khaali → `temp` NULL → crash. Koi list empty ho to pehle hi return — 21 mein dono-empty ke liye alag line zaroori hai (baaki checks `&& list2` wale hain), warna `l1->val` NULL deref.

## Related elsewhere
- [26. Remove Duplicates from Sorted Array](<../../Arrays/Two_Pointers/26. Remove Duplicates from Sorted Array.md>) — same read/write idea, array pe: value copy + write index.
- [Arrays two pointers](<../../Arrays/Two_Pointers/two_pointers.md>) — read/write pointers ke aur uses (80, 283).
- [Fast & Slow](<../Fast_Slow/fast_slow.md>) — same list pe alag speed / fixed gap wale pointers (876, 141, 19).
