class Solution {
public:
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
};