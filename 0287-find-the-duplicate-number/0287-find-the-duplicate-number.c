int findDuplicate(int* arr, int numsSnize) 
{
    // Floyd's cycle detection (tortoise-hare) use ho raha hai
    // array ko linked list samjho: index i -> arr[i] pe point karta hai
    // duplicate value ki wajah se do indices same node pe jaate hain = cycle banta hai
    int slow = 0;
    int fast = 0;

    // ---- Phase 1: cycle ke andar meeting point dhundo ----
    while (true)
    {
        slow = arr[slow];         // slow 1 step
        fast = arr[arr[fast]];    // fast 2 step

        // dono mil gaye = cycle ke andar kahin collision hua
        if (slow == fast)
            break;
    }

    // ---- Phase 2: cycle ka entry point dhundo (wahi duplicate hai) ----
    // fast ko start pe wapas le jao, slow wahi meeting point pe rehne do
    fast = 0;

    while (true)
    {
        // ab dono 1-1 step chalenge
        slow = arr[slow];
        fast = arr[fast];

        // jahan milenge wahi cycle ka entry = duplicate number
        if (slow == fast)
        {
            break;
        }
    }

    return fast;  // slow bhi same hota, dono equal hain
}