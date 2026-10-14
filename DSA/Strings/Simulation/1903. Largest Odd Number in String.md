class Solution {
public:
    string largestOddNumber(string num) 
    {
        int right = num.size()-1;
        string res;
        while(right>=0) 
        {
            if ((num[right] - '0') % 2 == 0) right --;
            else
            {
                res.append(num,0,right+1);
                break;
            }
        }   
        return res;
    }
};