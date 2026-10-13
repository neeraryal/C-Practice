/**
 * Definition for singly-linked list.
 * struct ListNode {
 *     int val;
 *     struct ListNode *next;
 * };
 */
typedef struct ListNode Node;
struct ListNode *detectCycle(struct ListNode *head) {
    Node * slow = head;
    Node * fast = head;

    while(fast && fast ->next)
    {
        slow = slow->next;
        fast = fast->next->next;

        if(slow == fast)
        {
            slow=head;
            while(slow !=fast)
            {
                slow =slow->next;
                fast = fast->next;
            }
                return slow ;
        }
    }
    return NULL;
}