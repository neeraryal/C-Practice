class Solution {
public:
    bool containsDuplicate(vector<int>& nums) 
    {
        unordered_set<int>mapp;
        for(auto it : nums)
        {
            if(mapp.count(it) > 0)
            {
                return true;
            }
            mapp.insert(it);
        }
        return false;
    }
};