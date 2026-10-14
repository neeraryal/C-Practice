class Solution {
public:
    string frequencySort(string s) 
    {
        unordered_map<char,int> freq;
        //Store frequency of each character 
        for (auto ch : s) freq[ch]++;
        //Create a bucket vector of vector of char , and fill it with map data.

        vector<vector <char>> bucket(s.size()+1);
        for( auto [ch, fr]: freq) bucket[fr].push_back(ch);

        string res;
        for (int i = bucket.size()-1; i>=1; i-- )
        {
            for(auto ch : bucket[i])
            {
                res.append(i,ch);
            }
        }

        return res;


    }
};



/*
    Approach 1 ::
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

    Approach 2::

    string frequencySort(string s) 
    {
        unordered_map<char,int> freq;
        //Store frequency of each character 
        for (auto ch : s) freq[ch]++;
        //Create a vector of pairs, and fill it with map data.
        vector<pair<char,int>> freq_arr;
        for( auto [ch, fq] : freq) freq_arr.push_back({ch,fq});

        auto cmp = [&](pair <char,int>&a, pair<char,int>&b)
        {
            return a.second > b.second;
        };
        //Sort the vector on the basis of frequency instead of char 
        sort(freq_arr.begin(),freq_arr.end(),cmp);
        //Append the charcter times frequency in the res.
        string res;
        for (auto [ch , fq] : freq_arr) res.append(fq,ch);

        return res;


    }
*/