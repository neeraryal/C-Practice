# Next Greater Element
> Har element ke right mein pehla bada element do, na ho to `-1`

| Pattern | Data structure | Time | Space |
|---|---|---|---|
| [Monotonic Stack – decreasing stack of indices](monotonic_stack.md) | Linked-list stack (indices store), `ans` array | O(n) | O(n) |

**How it works**
- **Soch:** Brute force mein har element ke liye right scan → O(n²). Ulta socho: jin elements ko abhi tak apna "next greater" nahi mila, unke indices stack mein wait karte hain. Koi bada element aaye to wahi un sabka answer hai.
- **Pop & answer:** Jab tak `arr[i] > arr[peek()]` → `pop` karo aur `ans[top_val] = arr[i]`.
- **Push:** Phir `push(i)` — ab `i` apne next greater ka wait karega.
- **Leftover:** Loop ke baad stack mein bache indices ke right mein koi bada nahi → `-1`.
- **Kyun sahi hai:** Stack (bottom → top) values decreasing (equal allowed) rehti hain, to `arr[i]` top se bada hai to wahi top ka pehla bada element hai (beech wale sab chhote the, pehle hi pop ho gaye). Har index ek baar push, ek baar pop → O(n).
- **Dry run:** `[2,1,2,4,3]` (stack values mein dikhaya, asal mein indices)
  - `2`, `1` push → stack `[2,1]`
  - `2`: `1` pop → ans[1]=2; push → `[2,2]`
  - `4`: dono `2` pop → ans[2]=4, ans[0]=4; push `4`, phir `3` push
  - leftover `4,3` → `-1` → `[4,2,4,-1,-1]` ✔

**Gotcha:** `>` strict hai → equal value "greater" nahi gini jaati (`[4,4]` → `[-1,-1]`). `top !=NULL` check `&&` mein pehle aata hai, isliye empty stack pe `peek()` ka `-1` kabhi `arr[-1]` nahi banta.
**vs related:** Folder mein akela problem — variants: next smaller ke liye `>` ko `<` karo, previous greater ke liye right se left scan.

<details><summary>Problem statement</summary>

Given an array arr[] of n integers, find the next greater element for every element. The next greater element of arr[i] is the first element to its right that is strictly greater than arr[i]. If no such element exists, the answer for that position is -1.

Example:

Input: arr[] = [2, 1, 2, 4, 3]
Output: [4, 2, 4, -1, -1]
Explanation: 2 → 4, 1 → 2, 2 → 4 (first greater element to the right); 4 and 3 have no greater element to their right → -1.

</details>

## Code

```c
/*
 * NEXT GREATER ELEMENT FINDER
 * 
 * Purpose: For each element in an array, finds the next greater element to its right.
 * 
 * Algorithm: Uses a monotonic decreasing stack approach
 * - Traverse the array from left to right
 * - Maintain a stack of indices where elements haven't found their next greater element yet
 * - For each new element:
 *   - Pop all smaller elements from stack and mark current element as their "next greater"
 *   - Push current element's index onto stack
 * - After traversal, any remaining indices in stack have no next greater element (marked as -1)
 * 
 * Time Complexity: O(n) - each element is pushed and popped at most once
 * Space Complexity: O(n) - for the stack and result array
 * 
 * Example: [2, 1, 2, 4, 3]
 * Result:  [4, 2, 4, -1, -1]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Node structure for implementing stack using linked list
typedef struct Node_t
{
    int data;              // Stores the index of array element
    struct Node_t *next;   // Pointer to next node in stack
} Node;

Node *top = NULL;  // Global pointer to top of stack

// Check if stack is empty
int isEmpty()
{
    return top == NULL; 
}

// Push a value (index) onto the stack
void push(int value)
{
    Node * newNode=(Node*) malloc (sizeof(Node));
    if(!newNode)
    {
        printf("Unable to allocate space for new node \n");
        return ; 
    }
    newNode->data= value;
    newNode->next = top;  // Point new node to current top
    top = newNode;         // Update top to new node
}

// Pop and return the top value from stack
int pop ()
{
    if(isEmpty ())
    {
        printf("stack is empty \n");
        return -1;
    }
    int value = top->data;
    Node* temp= top ;
    top = top->next;  // Move top to next node
    free(temp);       // Free the old top node
    return value; 
}

// Return the top value without removing it
int peek ()
{
    if(isEmpty())
    {
        printf("Stack is empty \n");
        return -1;
    }
    return top->data;
}

// Print all elements in the stack from top to bottom
void print_stack()
{
    if(isEmpty ())
    {
        printf("stack is empty \n");
        return ;
    }
    printf("Stack :");
    Node* temp= top; 
    while(temp)
    {
        printf(" %d ", (temp->data));
        temp=temp->next;
    }
    printf("\n");
}

int main()
{
    int n ;
    printf("Enter the size of the array : ");
    scanf("%d",&n);
    
    // Allocate memory for input array
    int *arr= (int*) malloc (n * sizeof(int));
    
    printf("Enter the values : ");
    for (int i=0 ;i<n;i++)
    {
        scanf("%d",&arr[i]);
    }
    
    // Display the input array
    printf("Array elements :");
    for (int i=0 ;i<n;i++)
    {
        printf(" %d ",arr[i]);
    }
    printf("\n");
    
    // Allocate result array initialized to 0
    int * ans = calloc(sizeof(int) ,n); 
    
    // Main algorithm: Find next greater element for each position
    for(int i=0 ; i<n ;i++)
    {
        // While stack is not empty and current element is greater than element at index stored at top
        while(top !=NULL  && arr[i]> arr[peek()])  // top!=NULL pehle → empty pe peek nahi; strict > → equal pop nahi
        {
            int top_val=pop();     // Pop the index from stack
            ans[top_val]=arr[i];   // Current element is the next greater for popped index
        }
        push(i);  // Push current index onto stack (waiting for its next greater element)
    }
    
    // Elements remaining in stack have no next greater element
    while(top !=NULL)
    {
        ans[peek()]=-1;  // Mark as -1 (no next greater element found)
        pop();           // Remove from stack
    }
    
    // Print the result array
    for (int i=0 ;i<n;i++)
    {
        printf(" %d ",ans[i]);
    }
    printf("\n");
    
    // Free allocated memory
    free(arr);
    free(ans);
    
    return 0; 
}
```
