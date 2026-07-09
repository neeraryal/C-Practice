class Solution {
public:
    bool isIsomorphic(string s, string t) {

        if(s.size() != t.size()) return false;

        char t_in_s[256]={0};
        char s_in_t[256]={0};

        int i=0;

        while(s[i]!='\0')
        {
            if(s_in_t[t[i]] !=0)
            {
                if(s_in_t[t[i]] != s[i]) return false;
            }
            if(t_in_s[s[i]] !=0 )
            {
                if(t_in_s[s[i]] != t[i]) return false;
            }
            s_in_t[t[i]] = s[i];
            t_in_s[s[i]] = t[i];
            i++;
        }
        return true;
    }
};