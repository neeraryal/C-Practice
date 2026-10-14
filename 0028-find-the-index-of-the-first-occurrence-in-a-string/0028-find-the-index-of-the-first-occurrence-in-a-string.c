int strStr(char* haystack, char* needle) 
{
    int s_h= strlen(haystack);
    int s_n= strlen(needle);

    if (s_n > s_h ) return -1;
    if ( s_n==0) return 0;
    int c_h=0;
    while (c_h <= s_h-s_n)
    {
        int c_n=0;
        if( haystack[c_h]==needle[c_n])
        {
            int temp_c_h = c_h;
            while (c_n < s_n)
            {
                if (haystack[temp_c_h]!=needle[c_n])
                {
                    break;
                }
                c_n++;
                temp_c_h++;
            }
            if( c_n == s_n) return c_h;
        }
        c_h++;
    }
    return -1;
}