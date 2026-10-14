class Solution {
public:
    int maxDepth(string s) {
        int res=0;
        int open_count=0; 
        for ( auto & ch : s)
        {
            if (ch=='(')
            {
                open_count++;
            }
            else if(ch==')')
            {
                open_count--;
            }
            res=max(res,open_count);
        }
        return res;
    }
};