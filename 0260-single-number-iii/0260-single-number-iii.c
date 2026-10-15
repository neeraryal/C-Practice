int* singleNumber(int* arr, int n, int* returnSize)
{
    // pehle se pata hai: do numbers ek baar aate hain (a, b), baaki sab exactly do baar
    // XOR trick: x ^ x = 0, x ^ 0 = x, to duplicates cancel ho jaate hain
    // unsigned isliye: neeche -xor karna hai, signed me INT_MIN pe UB hota
    unsigned int xor = 0;

    // ---- Step 1: saare elements ka XOR ----
    // duplicates cancel, bacha = a ^ b
    for (int i = 0; i < n; i++)
        xor ^= arr[i];

    // ---- Step 2: ek aisa bit dhundo jahan a aur b alag hain ----
    // a != b to a ^ b != 0, matlab kam se kam ek bit set hai
    // jahan 1 hai, wahan a aur b me se ek ka bit 0 aur doosre ka 1 hai
    uint32_t mask= xor & (~(xor-1));

    // result array heap pe (caller free karega), [a, b] store hoga
    int* res = malloc(2 * sizeof(int));
    res[0] = res[1] = 0;  // XOR accumulate karna hai, to 0 se start

    // ---- Step 3: mask se numbers ko 2 groups me baanto ----
    // group 1: mask wala bit set, group 2: bit clear
    // a aur b alag groups me jaayenge (kyunki is bit pe differ karte hain)
    // har duplicate pair same group me jaayega (same value = same bit), to cancel ho jaayega
    for (int i = 0; i < n; i++)
    {
        if (arr[i] & mask)
            res[1] ^= arr[i];   // set-bit group, bacha hua = ek unique number
        else
            res[0] ^= arr[i];   // clear-bit group, bacha hua = doosra unique number
    }

    *returnSize = 2;  // LeetCode ko batana hai ki array size 2 hai
    return res;
}