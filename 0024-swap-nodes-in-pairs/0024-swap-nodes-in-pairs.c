/**
 * Definition for singly-linked list.
 * struct ListNode {
 *     int val;
 *     struct ListNode *next;
 * };
 */
typedef struct ListNode Node;
struct ListNode* swapPairs(struct ListNode* head) 
{
    if( ! head || ! head->next) return head;

    Node * dummy = malloc( sizeof( Node));
    dummy -> next = head;

    Node * prev = dummy;

    while ( prev->next && prev->next->next )
    {
        Node * first = prev-> next;
        Node * second = first -> next ; 

        first->next=second->next;
        second->next= first;
        prev->next=second;

        prev=first; 
    }
    return dummy->next;
}