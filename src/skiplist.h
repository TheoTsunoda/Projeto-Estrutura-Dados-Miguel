#ifndef SKIPLIST_H
#define SKIPLIST_H

#include "dict.h"

/* ============================================================
 *  Skip List — probabilistic ordered dictionary (outside-syllabus #1)
 * ============================================================
 *
 * Keyed on tconst, exactly like the linked list / AVL / hash table, so it
 * joins the same valid benchmark group ("find a record by key").
 *
 * A skip list is a linked list with extra "express lanes": each node has a
 * tower of forward pointers whose height is chosen by coin flips. Searching
 * starts in the tallest lane and drops down a level whenever the next node
 * would overshoot the key, so most nodes are skipped. The expected height is
 * O(log n), giving expected O(log n) search/insert/remove WITHOUT the
 * rotations an AVL needs to stay balanced. That trade — randomness instead of
 * rebalancing — is the whole reason it is interesting next to the AVL.
 */
Dictionary skiplist_create(void);

#endif /* SKIPLIST_H */
