class Solution {
public:
    int expand_palindrome(string &s, int a, int b )
    {
        int plaindrome_length=0;

        while(a>=0 && b<s.size() && s[a]==s[b])
        {
            a--;
            b++;
        }
        return b-a-1;
    }

    string longestPalindrome(string s) 
    {
        int res_len=0;
        string res;
        int start =-1, end= -1;
        for (int i=0 ; i<s.size();i++)
        {
            int len1 = expand_palindrome(s, i, i);
            int len2 = expand_palindrome(s, i ,i+1);

            int len_max= max(len1, len2);
            if(len_max>res_len)
            {
                start = i-(len_max-1)/2;
                end= i+(len_max/2);
                res_len= len_max;
            }
        }
        res=s.substr(start, (end - start +1));
        return res; 
    }
};