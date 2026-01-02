// Online C compiler to run C program online
#include <stdio.h>
#include <stddef.h>
#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

typedef struct Device {
    int id;
    int state;
}Device;



int main() 
{
    Device dev;
    int *p = &dev.state;
    
    dev.id=2;
    dev.state=10;
    
    Device *back = (Device*)((char*)(p)-offsetof(Device,state));
        // (Device *)- return same pointer type
        // (char*)- We are substracting address of for 1 byte we type cast it to char, calculation will be coorect as
        // (p)-offsetof(Device,state) state's loaction - offset will point to start addres of structure 
        // container_of(p, struct Device, state);
        
// struct SensorData {
//     int id;           // Offset 0
//     char name[20];    // Offset 4
//     float value;      // Offset 24
// }; // Total size: 28 bytes (assuming no padding)
        
// Step,  Operation,               Memory Math (Example),  Result
// 1,      Get member pointer      ptr = 0x1018,           Pointer to value
// 2,      Find offset,           "offsetof(struct SensorData, value)",24 bytes
// 3,      Subtract,               0x1018 - 24,            0x1000
// 4,      Cast,(struct SensorData *)0x1000,               Full struct access
    
    printf("%d \n",back->id);
    printf("%d \n",back->state);

    
    if(back == &dev )
    {
        printf("Yes");
    }
    else
    printf("No");
    return 0;
}