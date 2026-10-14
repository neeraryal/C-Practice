int strStr(char* haystack, char* needle) 
{
    // dono strings ki length nikal li (strlen O(n) hai, ek baar hi call kar rahe hain)
    int s_h = strlen(haystack);
    int s_n = strlen(needle);

    // needle haystack se lamba hai to match possible hi nahi, seedha -1
    if (s_n > s_h) return -1;

    // empty needle ka matlab index 0 pe match (LeetCode ki convention)
    if (s_n == 0) return 0;

    int c_h = 0;  // haystack me current starting position

    // sirf s_h - s_n tak check karna hai, uske baad needle fit hi nahi hoga
    while (c_h <= s_h - s_n)
    {
        int c_n = 0;  // needle ka index, har naye start position pe reset

        // pehla char match hua to hi aage detail check karo (quick filter)
        if (haystack[c_h] == needle[c_n])
        {
            // c_h ko disturb nahi karna, isliye temp copy se aage badhenge
            int temp_c_h = c_h;

            // needle ke saare chars compare karo
            while (c_n < s_n)
            {
                // ek bhi char alag mila to is position pe match fail, break
                if (haystack[temp_c_h] != needle[c_n])
                {
                    break;
                }
                c_n++;        // needle me aage
                temp_c_h++;   // haystack me aage
            }

            // c_n poori needle length tak pahunch gaya = full match mil gaya
            if (c_n == s_n) return c_h;
        }

        // match nahi hua, to haystack me agli position try karo
        c_h++;
    }

    // poora haystack scan kar liya, needle nahi mila
    return -1;
}