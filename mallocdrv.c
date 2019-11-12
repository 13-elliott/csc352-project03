/*
	This is a minimal test case for malloc/free. This is NOT
	a complete, 100% coverage set of test cases. Your test cases
	should be much more complete. The value of this file is to
	remind you to test not just by calling your malloc and free
	implementations, but to also write to the memory that you allocate.

	If you don't write to the memory you allocate, the CPU and OS
	will never tell you your allocation wasn't correctly made by
	crashing. You just moved a pointer (brk) back and forth. It's
	in using the space that you test your allocations.

	To use, change the #defined values below from malloc to
	my_firstfit_malloc and my_free and change TIMES to a reasonable
	value. Compile this file with your C file containing the
	implementation of your malloc and free and link them together by:

	gcc -m32 -o malloctest mallocdrv.c mymalloc.c
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

//include your code
#include "mymalloc.h"

//replace malloc here with the appropriate version of mymalloc
#define MALLOC my_firstfit_malloc
//replace free here with the appropriate version of myfree
#define FREE my_free

//You can adjust how many things are allocated
#define TIMES 500

//If you want to make times bigger than 512, remove the call to qsort and do something else.
//Then remove this check.
#if TIMES >= 512
#error "TIMES is too big, qsort will switch to mergesort which requires a temporary malloc()ed array"
#endif

//Do not modify below here
struct tree {
    int data;
    struct tree *left;
    struct tree *right;
};

void freetree(struct tree *root) {
    if (root->left != NULL)
        freetree(root->left);
    if (root->right != NULL)
        freetree(root->right);
    FREE(root);
}

void randominsert(struct tree *head, struct tree *new) {
    int way;
    struct tree *curr, *prev;
    prev = NULL;
    curr = head;

    while (curr != NULL) {
        prev = curr;
        way = rand() % 2;
        if (way == 0) {
            curr = curr->left;
        } else {
            curr = curr->right;
        }
    }
    if (way == 0)
        prev->left = new;
    else
        prev->right = new;
}

void printtree(struct tree *head) {
    if (head == NULL)
        return;

    printtree(head->left);
    printf("%d\n", head->data);
    printtree(head->right);
}

void test1() {
    int i;

    struct tree *head = (struct tree *) MALLOC(sizeof(struct tree));
    head->data = 0;
    head->left = NULL;
    head->right = NULL;

    for (i = 1; i < TIMES; i++) {
        struct tree *new = (struct tree *) MALLOC(sizeof(struct tree));
        new->data = i;
        new->left = NULL;
        new->right = NULL;
        randominsert(head, new);
    }

    printtree(head);
    freetree(head);
}

int comp(const void *a, const void *b) {
    return *((int *) a) - *((int *) b);
}

void test2() {
    int *a;
    int i;

    a = (int *) MALLOC(TIMES * sizeof(int));

    for (i = 0; i < TIMES; i++) {
        a[i] = rand() % TIMES + 1;
    }

    // I suspect qsort implementation advances the brk
    qsort(a, TIMES, sizeof(int), comp);

    for (i = 0; i < TIMES; i++) {
        printf("%d\n", a[i]);
    }

    FREE(a);

}

// frees a random leaf node from the tree.
int randfree(struct tree *head) {
    if (head == NULL) {}
    else if (head->left == NULL && head->right == NULL) {
        FREE(head);
        return 1;
    } else if (rand() % 2 == 0 && head->left != NULL) {
        if (randfree(head->left))
            head->left = NULL;
    } else {
        if (randfree(head->right))
            head->right = NULL;
    }
    return 0;
}

// modified version of test1 which deallocates two random
// two leaf nodes after every 5 iterations.
void test3() {
    int i;

    struct tree *head = (struct tree *) MALLOC(sizeof(struct tree));
    head->data = 0;
    head->left = NULL;
    head->right = NULL;

    for (i = 1; i < TIMES; i++) {
        struct tree *new = (struct tree *) MALLOC(sizeof(struct tree));
        new->data = i;
        new->left = NULL;
        new->right = NULL;
        randominsert(head, new);
        if (i % 5 == 0) {
            randfree(head);
            randfree(head);
        }
    }

    printtree(head);
    freetree(head);
}

int main() {
    srand((unsigned int) time(NULL));

    //For default malloc, the number printed here will not match the two
    //below. For your malloc, all three should match. (The real malloc
    //steals some space that it never returns. Yours should always be
    //removing everything it creates and reducing brk).
    void *orig = sbrk(0);
    void *after;
    printf("original val brk: %p\n", orig);

    test1();
    after = sbrk(0);
    printf("brk after  test1: %p\n", after);
    assert(orig == after);

    test2();
    after = sbrk(0);
    printf("brk after  test2: %p\n", after);
    assert(orig == after);

    test3();
    after = sbrk(0);
    printf("brk after  test3: %p\n", after);
    assert(orig == after);

    return 0;
}
