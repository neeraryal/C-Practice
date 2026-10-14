/**
 * Definition for singly-linked list.
 * struct ListNode {
 *     int val;
 *     ListNode *next;
 *     ListNode() : val(0), next(nullptr) {}
 *     ListNode(int x) : val(x), next(nullptr) {}
 *     ListNode(int x, ListNode *next) : val(x), next(next) {}
 * };
 */
typedef ListNode Node;
class Solution {
public:
    bool isPalindrome(ListNode* head) 
    {
        if(!head || !head->next) return true;
        //1 Find the Middle 
        Node* slow = head;
        Node* fast = head;
        while(fast && fast->next)
        {
            slow=slow->next;
            fast=fast->next->next;
        }
        // Reverse second half
        Node* prev= NULL;
        Node* curr=slow; 
        while(curr)
        {
            ListNode* next= curr->next;
            curr->next= prev;
            prev=curr;
            curr=next;
        }

        //Compare both halves
        Node* right_head=prev;
        Node* left_head=head;
        while(right_head)
        {
            if(right_head->val != left_head->val) return false;
            right_head=right_head->next;
            left_head=left_head->next;
        }
        return true;
    }
};