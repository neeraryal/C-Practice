/**
 * Definition for singly-linked list.
 * struct ListNode {
 *     int val;
 *     struct ListNode *next;
 * };
 */

typedef struct ListNode Node;
bool hasCycle(struct ListNode *head)
{
    if ( !head || !head->next) return false ;
    Node * slow = head;
    Node * fast = head;

    while(fast && fast ->next)
    {
        slow = slow->next;
        fast = fast->next->next;

        if(slow == fast)
            return true ;
        
    }
    return false ;
}