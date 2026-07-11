class Solution {
public:
    string frequencySort(string s) 
    {
        unordered_map<char,int> freq;
        for (auto ch : s) freq[ch]++;

        vector<pair<char,int>> freq_arr;
        for( auto [ch, fq] : freq) freq_arr.push_back({ch,fq});

        auto cmp = [&](pair <char,int>&a, pair<char,int>&b)
        {
            return a.second > b.second;
        };

        sort(freq_arr.begin(),freq_arr.end(),cmp);

        string res;
        for (auto [ch , fq] : freq_arr) res.append(fq,ch);

        return res;


    }
};



/*
    string frequencySort(string s) 
    {
        vector<int>freq(128);
        //Store Frequency of each character in freq vector 
        for( auto ch : s)
        {
            freq[ch]++;
        }
        // Comparator function used for custom sorting on the basis of frequency, used lambda function
        auto cmp = [&](char a, char b){
            if(freq[a]==freq[b]) return a<b;
            return freq[a]>freq[b];
        };
        // Revserse sort string on the basis of frequency
        sort(s.begin(),s.end(), cmp);
        return s;

    }
*/