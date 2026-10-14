int findDuplicate(int* arr, int numsSnize) 
{
    int slow=0;
    int fast=0;

    while(true)
    {
        slow= arr[slow];
        fast= arr[arr[fast]];
        if( slow == fast)
        break;
    }

    fast=0;
    while(true)
    {
        slow= arr[slow];
        fast= arr[fast];
        if( slow == fast)
        {
            break;
        }
    }
    return fast;

}