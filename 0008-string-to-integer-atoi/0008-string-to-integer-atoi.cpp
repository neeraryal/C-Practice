class Solution {
public:
    int myAtoi(string s) 
    {
        int n= s.size();
        int i=0;
        int sign = 1;
        int res=0;
        while(s[i]==' ') i++;

        if(s[i] == '+' ||  s[i] == '-')
        {
            if(s[i]=='-')
                sign=-1;
            i++;
        }

        while(s[i]>= '0' && s[i]<='9')
        {
            int temp = s[i]-'0';

            if(res>INT_MAX/10 || (res== INT_MAX/10 && temp>7))
            {
                return sign==1?INT_MAX:INT_MIN;
            }

            res=(res*10)+temp;
            i++;
        }
        return res*sign;
        
    }
};