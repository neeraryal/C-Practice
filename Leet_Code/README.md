# LeetCode — Solutions by DSA Topic

33 problems solved, grouped by the core pattern used. Each folder holds the
solution (`.c` or `.md` with intuition + code).

| Topic | Count | Core idea |
|-------|:-----:|-----------|
| [Two Pointers](#two-pointers) | 10 | Two indices scanning inward / same-direction to avoid nested loops |
| [Sliding Window](#sliding-window) | 12 | Grow/shrink a window over a contiguous range; track a running aggregate |
| [Prefix Sum](#prefix-sum) | 3 | Precompute cumulative sums / running products for O(1) range queries |
| [Linked List](#linked-list) | 1 | Pointer manipulation on nodes |
| [Dynamic Programming](#dynamic-programming) | 2 | Build the answer from optimal sub-answers |
| [Arrays — Misc](#arrays--misc) | 5 | One-off array techniques (hashing, greedy, voting, math) |

---

## Two Pointers

| # | Problem | Note |
|---|---------|------|
| 11 | Container With Most Water | Move the shorter wall inward |
| 15 | 3Sum | Sort + fix one, two-pointer the rest |
| 16 | 3Sum Closest | Sort + two-pointer, track closest sum |
| 26 | Remove Duplicates from Sorted Array | Slow/fast write pointer |
| 75 | Sort Colors (0,1,2) | Dutch National Flag (3-way) |
| 80 | Remove Duplicates from Sorted Array II | Slow/fast, allow ≤2 |
| 167 | Two Sum II (Sorted Input) | Converge from both ends |
| 283 | Move All Zeroes | Same-direction write pointer |
| 844 | Backspace String Compare | Two pointers from the end |
| 977 | Squares of a Sorted Array | Fill from largest ends inward |

## Sliding Window

| # | Problem | Note |
|---|---------|------|
| 3 | Longest Substring Without Repeating Characters | Variable window + last-seen map |
| 76 | Minimum Window Substring | Shrink once all chars covered |
| 121 | Best Time to Buy and Sell Stock | One pass, track running min |
| 209 | Minimum Size Subarray Sum | Shrink while sum ≥ target |
| 424 | Longest Repeating Character Replacement | Window valid while len − maxFreq ≤ k |
| 485 | Max Consecutive Ones | Fixed streak count |
| 567 | Permutation in String | Fixed window + frequency match |
| 643 | Maximum Average Subarray I | Fixed-size window sum |
| 713 | Subarray Product Less Than K | Shrink while product ≥ k |
| 904 | Fruit Into Baskets | Longest window with ≤2 distinct |
| 1004 | Max Consecutive Ones III | Window with ≤k zeros |
| 2461 | Max Sum of Distinct Subarrays (Length K) | Fixed window + distinct set |

## Prefix Sum

| # | Problem | Note |
|---|---------|------|
| 238 | Product of Array Except Self | Prefix × suffix products |
| 560 | Subarray Sum Equals K | Running sum + hashmap of seen sums |
| 724 | Find Pivot Index | Total − left gives right |

## Linked List

| # | Problem | Note |
|---|---------|------|
| 83 | Remove Duplicates from Sorted List | Skip nodes equal to current |

## Dynamic Programming

| # | Problem | Note |
|---|---------|------|
| 53 | Maximum Subarray | Kadane's — best ending here |
| 509 | Fibonacci Number | Classic DP / memoization |

## Arrays — Misc

| # | Problem | Technique |
|---|---------|-----------|
| 1 | Two Sum | Hashing (complement lookup) |
| 66 | Plus One | Array + carry math |
| 122 | Best Time to Buy and Sell Stock II | Greedy (sum every rise) |
| 169 | Majority Element | Boyer–Moore voting |
| 581 | Shortest Unsorted Continuous Subarray | Left/right boundary scan |
