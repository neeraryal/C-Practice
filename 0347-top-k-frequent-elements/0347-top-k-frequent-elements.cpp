class Solution {
public:
    vector<int> topKFrequent(vector<int>& nums, int k) 
    {
        unordered_map<int, int> mapp;

        for(auto &it : nums)
        {
            mapp[it]++;
        }    

        priority_queue<pair<int,int>, vector<pair<int,int>>, greater<pair<int,int>>>maxpq;

        for(auto &[num, freq ] : mapp)
        {
            maxpq.push({freq,num});
            if(maxpq.size()>k)
            {
                maxpq.pop();
            }
        }
        vector<int>res;

        while(!maxpq.empty())
        {
            res.push_back(maxpq.top().second);
            maxpq.pop();
        }

        return res;

    }
};