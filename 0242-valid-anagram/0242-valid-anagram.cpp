class Solution {
public:
    bool isAnagram(string s, string t) 
    {
        int s_len = s.size();
        int t_len = t.size();

        if(s_len != t_len) return false;

        int freq[26]={0};
        int i=0;
        while(s[i] != '\0')
        {
            freq[(unsigned int)s[i]-'a']++;
            freq[(unsigned int)t[i]-'a']--;
            i++;   
        }
        i=0;
        while(i<26)
        {
            if(freq[i] !=0) return false;
            i++;
        }
        return true;
    }
};