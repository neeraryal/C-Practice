/**
 * Definition for singly-linked list.
 * struct ListNode {
 *     int val;
 *     struct ListNode *next;
 * };
 */
typedef struct ListNode Node ;
struct ListNode* mergeTwoLists(struct ListNode* list1, struct ListNode* list2) 
{
    if(!list1 && !list2) return NULL;
    if ( !list1 && list2 ) return list2;
    if ( !list2 && list1 ) return list1;

    Node * l1 = list1;
    Node * l2 = list2;

    if (l1->val > l2->val )
    {
        l1=list2;
        l2=list1;
    }
    Node * res= l1;
    while (l1 && l2)
    {
        Node* temp =NULL;
        while (l1 && l1->val <= l2->val )
        {
            temp = l1;
            l1= l1->next;
        }
        temp->next= l2;
        Node * swap_temp=NULL;
        swap_temp= l1;
        l1=l2;
        l2=swap_temp; 
    }
    return res;
}