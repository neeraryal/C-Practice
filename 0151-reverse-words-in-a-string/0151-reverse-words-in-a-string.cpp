class Solution {
public:
    string reverseWords(string s) 
    {
        string res="";
        int right= s.size()-1,left= s.size()-1;
        while(left>=0)
        {
            if(s[left]==' ')
            {
                left--;
                right=left;
                continue;
            }
            if(left>0 && s[left-1]==' ')
            {
                if(!res.empty())
                {
                    res.push_back(' ');
                }
                res.append(s,left,right-left+1);
                right=left;
            }
            else if (left==0 && s[left]!=' ')
            {
                if(!res.empty())
                {
                    res.push_back(' ');
                }
                res.append(s,left,right-left+1);
                right=left;
            }
            left--;
        }    
        return res;
    }
};