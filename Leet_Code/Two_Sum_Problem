/*
Given an array of integers nums and an integer target, return indices of the two numbers such that they add up to target.

Example 1:

Input: nums = [2,7,11,15], target = 9
Output: [0,1]
Explanation: Because nums[0] + nums[1] == 9, we return [0, 1].
Example 2:

Input: nums = [3,2,4], target = 6
Output: [1,2]
Example 3:

Input: nums = [3,3], target = 6
Output: [0,1]
*/

# define MAP_SIZE 20011

/* =========================
   Bucket structure (Chaining used for Collision)
   ========================= */
typedef struct Node{
    int key;
    int value;
    struct Node* next;
}Node;  

/* =========================
   Hash table (array of buckets)
   ========================= */
Node * MAP[MAP_SIZE];

/* =========================
   Initialize hash table
   ========================= */
void initTable(void) {
    for (int i = 0; i < MAP_SIZE; i++) {
        MAP[i] = NULL;
    }
}
/* =========================
   GET HASH/INDEX whee to put key 
   ========================= */
int get_hash(int key )
{
    return (key % MAP_SIZE + MAP_SIZE)% MAP_SIZE;
}

/* =========================
   Inset in MAP
   ========================= */
void insert(int key, int value)
{
    int key_index = get_hash(key);
    
    Node* current= MAP[key_index];
    
    //Check if key already exist, if yes then update 
    while(current !=NULL)
    {
        if(current -> key == key )
        {
            //usecase will define what to do for same key, here we are updating.
            current->value = value;
            return;
        }
        current = current->next ;
    }
    
    // There was no key present, so we insert anew one in head.
    Node * new_node = malloc(sizeof(Node));
    new_node->value = value;
    new_node->key = key;
    new_node->next= current;
    MAP[key_index]=new_node;
    
    return ;
}

/* =========================
   Search in MAP and return
   ========================= */

int* search(int key )
{
    int key_index = get_hash(key);
    
    Node * current = MAP[key_index];
    
    while(current != NULL)
    {
        if(current->key == key)
        {
            return &current->value;
        }
        current = current->next;
    }
    return NULL;
}

int* twoSum(int* arr, int n, int target, int* returnSize) 
{
    initTable();
    int res_size =2;
    *returnSize = res_size;
    int * res= malloc(sizeof(int) * res_size);

    for(int i=0 ; i< n ;i ++)
    {
        int other_pair = target - arr[i];
        int *other_pair_index = search(other_pair);
        if(other_pair_index != NULL)
        {
            res[0]=*other_pair_index;
            res[1]=i;
        }
        else 
        {
            insert(arr[i],i);
        }
    }
return res;
    
}