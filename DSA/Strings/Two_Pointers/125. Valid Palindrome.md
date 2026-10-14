class Solution {
public:
    bool check_is_alnum(char ch)
    {
        if(ch>= 'a' && ch<='z')
            return true;
        if(ch>= 'A' && ch<='Z')
            return true;
        if(ch>='0' && ch<='9')
            return true;
        
        return false;
    }

    char get_lower_case(char ch)
    {
        if(ch>='A' && ch <='Z')
        {
            return ch+32;
        }
        return ch;
    }

    bool isPalindrome(string s) 
    {
        int left = 0 , right = s.size()-1;
        while (left<right)
        {
            while(left<right && !check_is_alnum(s[left])) left++;
            while(left<right && !check_is_alnum(s[right])) right--;

            if(get_lower_case(s[left]) != get_lower_case(s[right]))
                return false;
            
            left++;
            right--;
        }
        
        return true;
    }
};