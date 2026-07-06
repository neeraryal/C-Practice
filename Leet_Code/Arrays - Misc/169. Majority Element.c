/*
Given an array nums of size n, return the majority element.

The majority element is the element that appears more than ⌊n / 2⌋ times. You may assume that the majority element always exists in the array.

 

Example 1:

Input: nums = [3,2,3]
Output: 3
Example 2:

Input: nums = [2,2,1,1,1,2,2]
Output: 2
 

Constraints:

n == nums.length
1 <= n <= 5 * 104
-109 <= nums[i] <= 109
The input is generated such that a majority element will exist in the array.
*/

int majorityElement(int* arr, int n) 
{
    int majority_count=0;
    int majority_element=0;

    for(int i=0 ;i<n;i++)
    {
        if(majority_count == 0 )
        {
            majority_element=arr[i];
        }
        if(majority_element==arr[i])
        {
            majority_count++;
        }
        else 
        {
            majority_count--;
        }
    }
    return majority_element;
}