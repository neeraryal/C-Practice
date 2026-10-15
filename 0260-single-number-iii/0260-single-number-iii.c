/**
 * Note: The returned array must be malloced, assume caller calls free().
 */
int* singleNumber(int* arr, int n, int* returnSize) 
{
    uint32_t xor=0;
    for( int i=0 ; i<n ;i++)
    {
        xor^=arr[i];
    }

uint32_t mask= xor & (~(xor-1));
int * res= malloc (sizeof(int) *2);
int k=0 , j=0 ;
for( int i=0 ; i<n ;i++)
{
    if(arr[i] & mask)
    {
        k^=arr[i];
    }
    else
    {
        j^=arr[i];
    }
}
res[0]=j;
res[1]=k;
*returnSize =2;
return res;

}