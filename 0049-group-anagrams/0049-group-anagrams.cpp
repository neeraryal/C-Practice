class Solution {
public:
    vector<vector<string>> groupAnagrams(vector<string>& strs) 
    {
        unordered_map<string,vector<string>> mapp;
        vector<vector<string>>res;

        for(auto &it : strs)
        {
            vector<int>str_freq(26,0);
            for (auto c: it)
            {
                str_freq[c-'a']++;
            }

            string key=" ";
            for ( int i=0 ; i< 26;i++)
            {
                key+=to_string(str_freq[i]);
                key+='#';
            }

            mapp[key].push_back(it);
        }    

        for( auto &it : mapp)
        {
            res.push_back(it.second);
        }
        return res;
    }
};