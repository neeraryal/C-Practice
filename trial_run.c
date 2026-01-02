#include <stdio.h>

typedef struct{
    int id;
    char c;            
    float value;
} Item;

void passing_const_ptr( Item *const p) //Can change the value it points but can not point someother 
{    
    printf("%d \n",p->id);
    p->id=11; //Works fine 

    Item p1={20,'f',2.0};
    Item *ptr= &p1;
    
    // p=ptr; // Will give error :- "assignment of read-only parameter 'p'"
}

void passing_pointer_to_const(Item const *p)//Can point some other address but can not change value using it 
{
    // p->id=11; //Will give error :- "assignment of member 'id' in read-only object"

    Item p1={20,'f',2.0};
    Item *ptr= &p1;
    
    p=ptr;
    printf("%f \n",p->value);

}
void passing_const_ptr_to_const_int(Item const * const p)
{
    // p->id=11; // error: assignment of member 'id' in read-only object

    Item p1={20,'f',2.0};
    Item *ptr= &p1;
    
    // p=ptr; //error: assignment of read-only parameter 'p'
    printf("%f \n",p->value);
}

int main() {
Item i={10,'d',3.14};

Item* ptr= &i;
passing_const_ptr(ptr);
printf(" After Calling Passsing const ptr function :%d \n",i.id);

passing_pointer_to_const(ptr);

passing_const_ptr_to_const_int(ptr);

return 0;
}