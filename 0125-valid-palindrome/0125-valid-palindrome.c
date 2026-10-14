bool check_is_al_num(char c)
{
    if ( c >= 'a' && c <= 'z')
    {
        return true; 
    }
    if ( c >= 'A' && c <= 'Z')
    {
        return true; 
    }
    if ( c >= '0' && c <= '9')
    {
        return true; 
    }
    return false;
}

char to_lower( char ch)
{
    if (ch >= 'A' && ch <= 'Z')
    {
        return (ch+('a'-'A'));
    }
    return ch; 
}

bool isPalindrome(char* s) 
{
    int left = 0 , right = strlen(s)-1;

    while( left<right)
    {
        while (left < right && !check_is_al_num(s[left]))left++;
        while (right > left && !check_is_al_num(s[right]))right--;

        if (to_lower(s[left]) != to_lower(s[right]))
        {
            return false;
        }
        left++;
        right--;
    }
    return true;
}