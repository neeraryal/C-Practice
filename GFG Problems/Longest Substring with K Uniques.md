You are given a string s consisting only lowercase alphabets and an integer k. Your task is to find the length of the longest substring that contains exactly k distinct characters.

Note : If no such substring exists, return -1. 

Examples:

Input: s = "aabacbebebe", k = 3
Output: 7
Explanation: The longest substring with exactly 3 distinct characters is "cbebebe", which includes 'c', 'b', and 'e'.
Input: s = "aaaa", k = 2
Output: -1
Explanation: There's no substring with 2 distinct characters.
Input: s = "aabaaab", k = 2
Output: 7
Explanation: The entire string "aabaaab" has exactly 2 unique characters 'a' and 'b', making it the longest valid substring.

```c

class Solution {
  public:
    int longestKSubstr(string &s, int k) 
    {
        int arr[26]={};
        int n= s.size();
        int left=0, right=0;
        int res=-1, window=0;
        int count =0 ;
        while (right< n)
        {
            
            while(count>k)
            {
                arr[s[left]-'a'] -=1;
                if(arr[s[left]-'a']==0) count --;
                left++;
            }
            if(arr[s[right]-'a']==0) count ++;
            arr[s[right]-'a'] +=1;
            window= right-left+1;
            if(count==k) res= res>window? res : window;
            right++;
        }
        return res;
        
    }
};

```