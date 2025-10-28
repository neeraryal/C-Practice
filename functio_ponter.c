#include <stdio.h>
 
#define BHAI 0
#define BHEN 1
 
typedef void (*cb_hello_print) (void);
cb_hello_print print_hello_ptr= NULL;
 
void greet_bhai(void)
{
    printf("Hello Bhai !!\n");
    return;
}
 
void greet_bhen(void)
{
    printf("Hello Bhen !!\n");
    return;
}
int chech_if_hello_print_callback_regitered()
{
    if(print_hello_ptr == NULL)
    {
        return 0;
    }
    else 
    {
        return 1;
    }
}
void register_hello_print_callback(cb_hello_print cb)
{
    print_hello_ptr= cb;
}
void de_register_hello_print_callback()
{
    print_hello_ptr=NULL;
}
 
void greetings()
{
    printf("Hi, ");
    if(chech_if_hello_print_callback_regitered())
        print_hello_ptr();
    else 
        printf("\nRegister the callback first.!! \n");
    return;
}
 
int main() {
    register_hello_print_callback(greet_bhai);
    greetings();
    de_register_hello_print_callback();
    register_hello_print_callback(greet_bhen);
    greetings();
    return 0;
}
