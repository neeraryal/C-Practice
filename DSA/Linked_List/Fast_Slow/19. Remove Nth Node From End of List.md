/**
 * Definition for singly-linked list.
 * struct ListNode {
 *     int val;
 *     struct ListNode *next;
 * };
 */
typedef struct ListNode Node;
struct ListNode* removeNthFromEnd(struct ListNode* head, int n) {
    Node * dummy= malloc ( sizeof(Node));
    dummy-> next=head;
    Node * s = dummy , * f = dummy; 
    for(int i=0 ; i< n ; i++)
    {
        f= f->next ;
    }

    while (f->next)
    {
        f=f->next;
        s=s->next;
    }

    s->next =s->next->next;
    return dummy->next;
}