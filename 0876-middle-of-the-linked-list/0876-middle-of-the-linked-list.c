/**
 * Definition for singly-linked list.
 * struct ListNode {
 *     int val;
 *     struct ListNode *next;
 * };
 */
typedef struct ListNode Node;
struct ListNode* middleNode(struct ListNode* head) 
{
    Node *s = head, *f = head;

    while (f && f->next)
    {
        s= s->next;
        f= f->next->next;
    }
    return s;
}