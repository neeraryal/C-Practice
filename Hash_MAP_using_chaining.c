/*
 * HASH MAP IMPLEMENTATION USING SEPARATE CHAINING
 * 
 * Purpose: Implements a hash table data structure that maps integer keys to integer values
 * 
 * Key Concepts:
 * - Hash Function: Converts a key into an array index using modulo operation
 * - Collision Resolution: Uses separate chaining (linked lists) to handle multiple keys
 *   that hash to the same index
 * - Operations Supported: Insert (with update), Search
 * 
 * How it works:
 * 1. Hash function converts any integer key into a valid array index (0 to MAP_SIZE-1)
 * 2. Each array position (bucket) holds a linked list of key-value pairs
 * 3. When collision occurs (two keys hash to same index), new node is added to the list
 * 4. Search operation: Hash the key → traverse the linked list at that index → find matching key
 * 
 * Time Complexity:
 * - Average case: O(1) for insert and search
 * - Worst case: O(n) if all keys collide into same bucket
 * 
 * Example:
 * - Key 10 → hash(10) = 10 % 20011 = 10 → stored at MAP[10]
 * - Key 110 → hash(110) = 110 % 20011 = 110 → stored at MAP[110]
 * - Key -90 → hash(-90) = (-90 % 20011 + 20011) % 20011 = 19921 → stored at MAP[19921]
 */

#include <stdio.h>
#include <stdlib.h>

// Prime number size for better hash distribution and fewer collisions
# define MAP_SIZE 20011

/* =========================
   BUCKET NODE STRUCTURE
   Each node stores one key-value pair
   Multiple nodes at same index form a linked list (chain)
   ========================= */
typedef struct Node{
    int key;              // The key to be hashed
    int value;            // The value associated with the key
    struct Node* next;    // Pointer to next node in the chain (for collision handling)
}Node;  

/* =========================
   HASH TABLE DECLARATION
   Array of pointers to Node (each index is a bucket/chain head)
   ========================= */
Node * MAP[MAP_SIZE];

/* =========================
   INITIALIZE HASH TABLE
   Sets all buckets to NULL (empty hash table)
   Must be called before using the hash map
   ========================= */
void initTable(void) {
    for (int i = 0; i < MAP_SIZE; i++) {
        MAP[i] = NULL;  // No chains exist initially
    }
}

/* =========================
   HASH FUNCTION
   Converts any integer key into a valid array index [0, MAP_SIZE-1]
   
   Formula: (key % MAP_SIZE + MAP_SIZE) % MAP_SIZE
   
   Why this formula?
   - key % MAP_SIZE: Basic modulo operation
   - + MAP_SIZE: Handles negative keys (makes result positive)
   - % MAP_SIZE again: Ensures final result is within bounds
   
   Example:
   - key = 10  → (10 % 20011 + 20011) % 20011 = 10
   - key = -90 → (-90 % 20011 + 20011) % 20011 = 19921
   ========================= */
int get_hash(int key )
{
    return (key % MAP_SIZE + MAP_SIZE) % MAP_SIZE;
}

/* =========================
   INSERT/UPDATE OPERATION
   
   Steps:
   1. Calculate hash index for the key
   2. Traverse the chain at that index
   3. If key already exists → update its value
   4. If key doesn't exist → create new node at head of chain
   
   Time Complexity: O(chain_length) - typically O(1) with good hash function
   ========================= */
void insert(int key, int value)
{
    // Step 1: Get the bucket index where this key should go
    int key_index = get_hash(key);
    
    // Step 2: Start from the head of the chain at this bucket
    Node* current = MAP[key_index];
    
    // Step 3: Check if key already exists in the chain
    while(current != NULL)
    {
        if(current->key == key)
        {
            // Key found - UPDATE the existing value
            current->value = value;
            return;  // Exit after update
        }
        current = current->next;  // Move to next node in chain
    }
    
    // Step 4: Key not found - INSERT a new node at the head of the chain
    Node * new_node = malloc(sizeof(Node));
    new_node->value = value;
    new_node->key = key;
    new_node->next = MAP[key_index];  // Point new node to current head
    MAP[key_index] = new_node;        // Make new node the new head
    
    return;
}

/* =========================
   SEARCH OPERATION
   
   Steps:
   1. Calculate hash index for the key
   2. Traverse the chain at that index
   3. If key is found → return pointer to its value
   4. If key is not found → return NULL
   
   Returns: Pointer to value (allows checking if key exists and getting value)
   
   Time Complexity: O(chain_length) - typically O(1) with good hash function
   ========================= */
int* search(int key)
{
    // Step 1: Get the bucket index where this key should be
    int key_index = get_hash(key);
    
    // Step 2: Start from the head of the chain at this bucket
    Node * current = MAP[key_index];
    
    // Step 3: Traverse the chain looking for the key
    while(current != NULL)
    {
        if(current->key == key)
        {
            // Key found - return pointer to its value
            return &current->value;
        }
        current = current->next;  // Move to next node in chain
    }
    
    // Step 4: Key not found in the chain
    return NULL;
}

/* =========================
   MAIN FUNCTION - DEMONSTRATION
   Shows how hash map handles:
   - Basic insertion
   - Collision (keys 10 and 110 might collide depending on MAP_SIZE)
   - Negative keys
   - Updating existing keys
   - Searching for keys
   ========================= */
int main() {
    // Initialize empty hash table
    initTable();
    
    // Insert key-value pairs
    insert(10, 100);     // Insert: key=10, value=100
    insert(110, 200);    // Insert: key=110, value=200 (may collide with 10)
    insert(-90, 300);    // Insert: key=-90, value=300 (tests negative key handling)
    
    // Search for key 10 and print its value
    int* val = search(10);
    if (val) printf("Key 10 → %d\n", *val);  // Expected: 100
    
    // Update existing key 10 with new value
    insert(10, 999);     // Update: key=10, new value=999
    
    // Search again to verify update
    val = search(10);
    if (val) printf("Key 10 → %d\n", *val);  // Expected: 999
    
    // Search for negative key
    val = search(-90);
    if (val) printf("Key -90 → %d\n", *val); // Expected: 300
    
    return 0;
}

/* =========================
   EXPECTED OUTPUT:
   Key 10 → 100
   Key 10 → 999
   Key -90 → 300
   ========================= */