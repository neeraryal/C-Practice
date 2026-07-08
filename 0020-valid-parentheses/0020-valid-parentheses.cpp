class Solution {
public:
    bool isValid(string s) 
    {
        stack<char> st;
        int i=0 ;
        while(i<s.size())
        {
            if(s[i]=='(' | s[i] == '{' | s[i] == '[')  
                st.push(s[i]);
            else
            {
                switch(s[i])
                {
                    case '}' :
                    {
                        if(!st.empty() && st.top() == '{')
                            st.pop();
                        else
                            return false;
                        break;
                    }
                    case ']' :
                    {
                        if(!st.empty() && st.top() == '[')
                            st.pop();
                        else
                            return false;
                        break;
                    }
                    case ')' :
                    {
                        if(!st.empty() && st.top() == '(')
                            st.pop();
                        else 
                            return false;
                        break;
                    }
                }
            }
            i++;
        }
        if(!st.empty()) return false;

        return true;
    }
};