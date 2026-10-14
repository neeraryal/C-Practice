class Solution {
public:
    int longestConsecutive(vector<int>& nums) 
    {
        int size=nums.size();
        if(size==0) return 0;
        set<int>s;
        for(auto it :nums)
        {
            s.emplace(it);
        }
        int res=0;
        for(auto it : nums)
        {
            if(!s.count(it-1))
            {
                int count=1 ,current=it;
                while(s.count(current+1))
                  {
                      count+=1;
                      current+=1;
                  }
                  res=max(res,count);
             }
      }          
       return res;
    }
};