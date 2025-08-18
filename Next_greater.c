#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct Node_t
{
    int data;
    struct Node_t *next;
} Node;

Node *top = NULL;

int isEmpty()
{
    return top == NULL; 
}

void push(int value)
{
    Node * newNode=(Node*) malloc (sizeof(Node));
    if(!newNode)
    {
        printf("Unable to allocate space for new node \n");
        return ; 
    }
    newNode->data= value;
    newNode->next = top;
    top = newNode;
}

int pop ()
{
    if(isEmpty ())
    {
        printf("stack is empty \n");
        return -1;
    }
    int value = top->data;
    Node* temp= top ;
    top = top->next;
    free(temp);
    return value; 
}

int peek ()
{
    if(isEmpty())
    {
        printf("Stack is empty \n");
        return -1;
    }
    return top->data;
}

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

    int *arr= (int*) malloc (n * sizeof(int));
    printf("Enter the values : ");
    for (int i=0 ;i<n;i++)
    {
        scanf("%d",&arr[i]);
    }
    printf("Array elements :");
    for (int i=0 ;i<n;i++)
    {
        printf(" %d ",arr[i]);
    }
    printf("\n");  //2,1,2,4,3

    int * ans = calloc(sizeof(int) ,n); 
    for(int i=0 ; i<n ;i++)
    {
        while(top !=NULL  && arr[i]> arr[peek()])
        {
            int top_val=pop();
            ans[top_val]=arr[i];
        }
        push(i);
    }
    
    while(top !=NULL)
    {
        ans[peek()]=-1;
        pop();
    }

    for (int i=0 ;i<n;i++)
    {
        printf(" %d ",ans[i]);
    }

    printf("\n");

    free(arr);
    free(ans);
    return 0; 
}