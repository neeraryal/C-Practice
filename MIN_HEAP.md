# Min Heap – Core Operations

## What is a Min Heap?

A **Min Heap** is a **complete binary tree** where:

* Every **parent node ≤ its children**
* The **minimum element is always at the root**
* The tree is stored efficiently using an **array**

This structure guarantees fast access to the minimum element while keeping insertions and deletions efficient.

---

## Array Representation (0-Indexed)

For an element at index `i`:

* **Parent index** → `(i - 1) / 2`
* **Left child index** → `2*i + 1`
* **Right child index** → `2*i + 2`

The heap is **not globally sorted**.
Only the **parent–child relationship** is guaranteed.

---

## 1. Insert (Push)

### Goal

Insert a new element while preserving:

* Completeness of the tree
* Min Heap property

---

### Steps

1. **Insert the new element at the end of the array**
   (Maintains complete binary tree property)

2. **Heapify Up (Sift Up / Bubble Up)**

   * Compare the element with its parent
   * Swap if the element is smaller
   * Repeat until heap property is restored

---

### Example

Insert `2` into the heap:

```
Initial Heap: [1, 3, 6, 5, 9, 8]

Step 1: Append
[1, 3, 6, 5, 9, 8, 2]

Step 2: Heapify Up
[1, 3, 2, 5, 9, 8, 6]
```

---

### Time Complexity

* **O(log n)**
  Reason: In the worst case, the element moves up the height of the tree.

---

## 2. Extract Min (Pop)

### Goal

Remove and return the minimum element (root) while preserving heap structure.

---

### Steps

1. **Save the root value** (this is the minimum)
2. **Move the last element to the root**
3. **Remove the last element**
4. **Heapify Down (Sift Down)**

   * Compare with both children
   * Swap with the smaller child
   * Repeat until heap property holds

---

### Example

```
Initial Heap: [1, 3, 2, 5, 9, 8, 6]

Remove 1
Move last element to root:
[6, 3, 2, 5, 9, 8]

Heapify Down:
[2, 3, 6, 5, 9, 8]
```

---

### Time Complexity

* **O(log n)**
  Reason: In the worst case, the element moves down the height of the tree.

---

## 3. Peek (Get Min)

### Goal

Access the minimum element without modifying the heap.

---

### Steps

* Return `heap[0]`

---

### Time Complexity

* **O(1)**
  Reason: Direct array access.

---

## 4. Heapify (Build Heap)

### Goal

Convert an **unsorted array** into a valid Min Heap.

---

### Correct Approach (Bottom-Up)

1. Start from the **last non-leaf node**
2. Apply **Heapify Down** on each node
3. Continue moving upward to the root

---

### Index of Last Non-Leaf Node

```
(n / 2) - 1
```

All indices after this are leaf nodes and already satisfy the heap property.

---

### Time Complexity

* **O(n)**
  This is **not** `O(n log n)`

**Why?**

* Most nodes are near the leaves (small heapify cost)
* Only a few nodes are near the root (high cost)
* Total work sums to linear time

---

## Key Takeaways (Must Remember)

* Min Heap is **not sorted**
* Only the **minimum element** is guaranteed at the root
* Insert → **Heapify Up**
* Extract Min → **Heapify Down**
* Build Heap → **Bottom-up**, not repeated insertions
* Array representation enables efficient memory and cache usage

---

```c
#include <stdio.h>
#include <stdlib.h>

#define HEAP_CAPACITY 10

typedef struct 
{
    int *arr;
    int size; 
    int capacity;
}MinHeap;

int parent(int i) {return (i-1)/2;}
int left_child(int i) {return (2*i)+1;}
int right_child(int i) {return (2*i)+2;}

MinHeap* create_min_heap()
{
    MinHeap* min_heap= (MinHeap*) calloc(1,sizeof(MinHeap));

    min_heap-> capacity= HEAP_CAPACITY;
    min_heap->size=0;
    min_heap->arr= (int*) calloc(HEAP_CAPACITY,sizeof(int));

    if(!min_heap->arr)
    {
        free(min_heap);
        return NULL;
    }
    return min_heap;
}

void heapify_up(MinHeap* min_heap, int index)
{
   while(index>0)
   {
        int parent_index= parent(index);
        if(min_heap->arr[parent_index]<=min_heap->arr[index])
        {
            return;
        }
        int temp;
        temp=min_heap->arr[index];
        min_heap->arr[index]= min_heap->arr[parent_index];
        min_heap->arr[parent_index]=temp;

        index=parent_index;//Don't forget to change the index with the parent index as we have swapped them now.
   }
}

int insert(MinHeap* min_heap, int value)
{
    if(min_heap->size == min_heap->capacity)
        return -1; //As heap is already full, inserting this will lead to hep overflow
    
    min_heap->arr[min_heap->size]= value;
    heapify_up(min_heap, min_heap->size);
    min_heap->size++;

    return 0;
}

void heapify_down(MinHeap* min_heap, int index)
{
    while(1)
    {
        int left= left_child(index);
        int right = right_child(index);
        int smallest = index;

        if(left < min_heap->size && min_heap->arr[smallest] > min_heap->arr[left])
        {
            smallest=left;
        }

        if(right < min_heap->size && min_heap->arr[smallest] > min_heap->arr[right])
        {
            smallest=right;
        }

        if(smallest == index ) break;

        int temp = min_heap->arr[smallest];
        min_heap->arr[smallest] = min_heap->arr[index];
        min_heap->arr[index]=temp;

        index= smallest;

    }
}

int extract_min(MinHeap* min_heap )
{
    if(min_heap->size == 0 ) return -1;

    int out=min_heap->arr[0];

    min_heap->arr[0]=min_heap->arr[min_heap->size-1];
    min_heap->size--;
    heapify_down(min_heap, 0);

    return out ;
}

int peekMin(MinHeap *min_heap, int *out)
{
    if (min_heap->size == 0)
        return -1;

    *out = min_heap->arr[0];
    return 0;
}

void printHeap(MinHeap *min_heap)
{
    for (int i = 0; i < min_heap->size; i++)
        printf("%d ", min_heap->arr[i]);
    printf("\n");
}

int main()
{
    MinHeap *heap = create_min_heap();

    insert(heap, 5);
    insert(heap, 3);
    insert(heap, 8);
    insert(heap, 1);
    insert(heap, 2);

    printHeap(heap);

    printf("Min: %d\n", extract_min(heap));
    printHeap(heap);

    free(heap->arr);
    free(heap);
    return 0;
}
```
## Better Commented Code using AI's help 

```c
#include <stdio.h>
#include <stdlib.h>

// Define the maximum capacity of the heap
#define HEAP_CAPACITY 10

// MinHeap structure to hold heap data
typedef struct 
{
    int *arr;      // Dynamic array to store heap elements
    int size;      // Current number of elements in the heap
    int capacity;  // Maximum number of elements the heap can hold
} MinHeap;

// Helper function to get the parent index of a node at index i
// In a binary heap, parent of node at index i is at (i-1)/2
int parent(int i) { return (i - 1) / 2; }

// Helper function to get the left child index of a node at index i
// In a binary heap, left child of node at index i is at (2*i)+1
int left_child(int i) { return (2 * i) + 1; }

// Helper function to get the right child index of a node at index i
// In a binary heap, right child of node at index i is at (2*i)+2
int right_child(int i) { return (2 * i) + 2; }

// Function to create and initialize a new min heap
MinHeap* create_min_heap()
{
    // Allocate memory for the MinHeap structure and initialize to zero
    MinHeap* min_heap = (MinHeap*) calloc(1, sizeof(MinHeap));

    // Set the capacity to the predefined maximum
    min_heap->capacity = HEAP_CAPACITY;
    
    // Initialize size to 0 (heap is empty initially)
    min_heap->size = 0;
    
    // Allocate memory for the array that will store heap elements
    min_heap->arr = (int*) calloc(HEAP_CAPACITY, sizeof(int));

    // Check if array allocation failed
    if (!min_heap->arr)
    {
        free(min_heap);  // Clean up the heap structure if array allocation failed
        return NULL;      // Return NULL to indicate failure
    }
    
    return min_heap;  // Return pointer to the created heap
}

// Function to restore min heap property by moving an element up the tree
// This is called after inserting a new element at the end of the heap
void heapify_up(MinHeap* min_heap, int index)
{
    // Continue moving up the tree until we reach the root (index 0)
    while (index > 0)
    {
        // Get the parent index of the current node
        int parent_index = parent(index);
        
        // If parent is smaller or equal, heap property is satisfied
        if (min_heap->arr[parent_index] <= min_heap->arr[index])
        {
            return;  // Stop here, heap property is restored
        }
        
        // Parent is larger than current node, so swap them
        int temp;
        temp = min_heap->arr[index];
        min_heap->arr[index] = min_heap->arr[parent_index];
        min_heap->arr[parent_index] = temp;

        // Move up to the parent's position and continue checking
        index = parent_index;
    }
}

// Function to insert a new value into the min heap
int insert(MinHeap* min_heap, int value)
{
    // Check if the heap is already full
    if (min_heap->size == min_heap->capacity)
        return -1;  // Return -1 to indicate insertion failed (heap overflow)
    
    // Insert the new value at the end of the array (next available position)
    min_heap->arr[min_heap->size] = value;
    
    // Restore the min heap property by moving the new element up if needed
    heapify_up(min_heap, min_heap->size);
    
    // Increment the size since we added a new element
    min_heap->size++;

    return 0;  // Return 0 to indicate successful insertion
}

// Function to restore min heap property by moving an element down the tree
// This is called after removing the root element
void heapify_down(MinHeap* min_heap, int index)
{
    // Continue until we find the correct position for the element
    while (1)
    {
        // Get indices of left and right children
        int left = left_child(index);
        int right = right_child(index);
        
        // Assume current index has the smallest value
        int smallest = index;

        // If left child exists and is smaller than current smallest
        if (left < min_heap->size && min_heap->arr[smallest] > min_heap->arr[left])
        {
            smallest = left;  // Update smallest to left child
        }

        // If right child exists and is smaller than current smallest
        if (right < min_heap->size && min_heap->arr[smallest] > min_heap->arr[right])
        {
            smallest = right;  // Update smallest to right child
        }

        // If the current index is already the smallest, heap property is satisfied
        if (smallest == index) 
            break;  // Stop here, we're done

        // Swap current element with the smallest child
        int temp = min_heap->arr[smallest];
        min_heap->arr[smallest] = min_heap->arr[index];
        min_heap->arr[index] = temp;

        // Move down to the position of the smallest child and continue
        index = smallest;
    }
}

// Function to remove and return the minimum element (root) from the heap
int extract_min(MinHeap* min_heap)
{
    // Check if heap is empty
    if (min_heap->size == 0) 
        return -1;  // Return -1 to indicate heap is empty

    // Save the minimum value (root) to return later
    int out = min_heap->arr[0];

    // Replace root with the last element in the heap
    min_heap->arr[0] = min_heap->arr[min_heap->size - 1];
    
    // Decrease the size since we're removing an element
    min_heap->size--;
    
    // Restore min heap property by moving the new root down if needed
    heapify_down(min_heap, 0);

    return out;  // Return the minimum value that was extracted
}

// Function to peek at the minimum element without removing it
int peekMin(MinHeap *min_heap, int *out)
{
    // Check if heap is empty
    if (min_heap->size == 0)
        return -1;  // Return -1 to indicate heap is empty

    // Store the minimum value (root) in the output parameter
    *out = min_heap->arr[0];
    
    return 0;  // Return 0 to indicate success
}

// Function to print all elements in the heap array (level-order)
void printHeap(MinHeap *min_heap)
{
    // Iterate through all elements in the heap
    for (int i = 0; i < min_heap->size; i++)
        printf("%d ", min_heap->arr[i]);
    
    printf("\n");  // Print newline after all elements
}

// Main function to demonstrate heap operations
int main()
{
    // Create a new min heap
    MinHeap *heap = create_min_heap();

    // Insert several values into the heap
    insert(heap, 5);
    insert(heap, 3);
    insert(heap, 8);
    insert(heap, 1);
    insert(heap, 2);

    // Print the heap after insertions
    // Output will be: 1 2 8 5 3 (heap array representation, not sorted)
    printHeap(heap);

    // Extract and print the minimum element
    printf("Min: %d\n", extract_min(heap));  // Prints: Min: 1
    
    // Print the heap after extraction
    // Output will be: 2 3 8 5 (the new minimum is now at the root)
    printHeap(heap);

    // Free allocated memory to prevent memory leaks
    free(heap->arr);   // Free the array
    free(heap);        // Free the heap structure
    
    return 0;
}
```