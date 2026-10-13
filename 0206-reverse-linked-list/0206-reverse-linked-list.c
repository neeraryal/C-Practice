/**
 * Definition for singly-linked list.
 * struct ListNode {
 *     int val;
 *     struct ListNode *next;
 * };
 */
typedef struct ListNode Node; 
struct ListNode* reverseList(struct ListNode* head) 
{
    Node * current =head;
    Node * next =NULL;
    Node * prev = NULL;
    while ( current )
    {
        next= current->next;
        current -> next= prev;
        prev= current;
        current= next;
    }
    return prev; 
}