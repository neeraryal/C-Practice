class Solution {
public:
    string removeOuterParentheses(string s) 
    {
        string res ="";
        int left = 0, right = 0;
        int open_count=0;
        while(right<s.size())
        {
            if(s[right]=='(')
                open_count++;
            else
                open_count--;

            if(open_count ==0)
            {
                res.append(s,left+1, right-left-1);
                left=right+1;
            }
            right++;
        }
        return res;
    }
};