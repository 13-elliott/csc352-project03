#include <stdio.h>
#include <unistd.h>
#define NDEBUG
#include <assert.h>

#define TRUE 1
#define FALSE 0
#define BREAK_INVARIANT ltail == NULL \
    || ((void *) ltail + ltail->size + sizeof(Node) == sbrk(0))

typedef int bool;

typedef struct Node {
    int size;
    bool is_free;
    struct Node *prev;
    struct Node *next;
} Node;

// Prototypes
void *my_firstfit_malloc(int);
Node *allocate_at_break(int);
void allocate_at_node(Node *, int);
Node *firstfit_find(int);
void my_free(void *);
void coalesce(Node *);
void shrink_brk();
void insert_after(Node *, Node *);
void remove_node(Node *);

// global vars, head and tail of the list
Node *lhead = NULL;
Node *ltail = NULL;

/* My implementation of stdlib's malloc using the "firstfit" algorithm.
 * Allocates memory of the given size (plus sizeof(Node)) and returns
 * the address of a memory reigon of the requested size
 */
void *my_firstfit_malloc(int size) {
    Node *selected;
    if (lhead == NULL) {
        // make the first allocation
        selected = allocate_at_break(size);
        selected->prev = selected->next = NULL;
        lhead = ltail = selected;
    } else {
        // search for a fit
        selected = firstfit_find(size);
        if (selected == NULL) {
            // if no fit found, push the brk forward
            selected = allocate_at_break(size);
            insert_after(ltail, selected);
        } else {
            allocate_at_node(selected, size);
        }
    }
    assert(BREAK_INVARIANT);
    // ptr arith: return the address just after the Node
    return selected + 1;
}

/* Pushes the break forward enough to make the requested allocation.
 * returns a pointer to a newly created Node (at the previous brk)
 */
Node *allocate_at_break(int request_size) {
    Node *new = sbrk(sizeof(Node) + request_size);
    new->is_free = FALSE;
    new->size = request_size;
    return new;
}

/* Allocates memory using the given free Node. If there is space
 * for another allocation in the space remaining after this request
 * within the selected Node, then such a node is created and inserted
 * after the given Node in the list.
 */
void allocate_at_node(Node *selected, int request_size) {
    assert(selected != NULL);
    assert(selected->is_free);
    assert(selected->size >= request_size);

    // include space for another node in what would be leftover
    int leftover_space = selected->size - request_size - sizeof(Node);

    selected->is_free = FALSE;
    if (leftover_space > 0) {
        // create new free node at the end of the request
        Node *new = (void *) (selected + 1) + request_size;
        selected->size = request_size;
        new->is_free = TRUE;
        // note: Node size already compensated for in leftover_space
        new->size = leftover_space;
        insert_after(selected, new);
    }
}

/* Returns the node associated with the first space in which the
 * requested allocation can be fit. returns NULL if no fit can be found.
 */
Node *firstfit_find(int request_size) {
    Node *curr = lhead;
    while (curr != NULL) {
        if (curr->is_free && curr->size >= request_size) {
            // exits loop to the return stmt while curr refers
            // to the desired node
            break;
        }
        curr = curr->next;
    }
    return curr;
}

/* My implementation of stdlib's free.
 * Given location pointer MUST be an address
 * returned by my_firstfit_malloc which has
 * not already been freed. Behavior is otherwise undefined.
 */
void my_free(void *location) {
    Node *to_free = location - sizeof(Node);
    assert(!to_free->is_free);

    to_free->is_free = TRUE;
    // This assert fails after qsort is called in
    // mallocdrv.c when TIMES >= 256. Thus I suspect
    // the given 512 limit is erroneus(?)
    assert(BREAK_INVARIANT);
    coalesce(to_free);
    assert(BREAK_INVARIANT);
    shrink_brk();
    assert(BREAK_INVARIANT);
}

/* Considers the the given Node and the Nodes immediately
 * before and after it in the list. If more than one of those
 * are free, removes the latter free Node(s) from the list
 * and gives their space to leftmost free node of those three.
 * (reflected in that Node's size field)
 */
void coalesce(Node *selected) {
    Node *prev = selected->prev;
    Node *next = selected->next;

    if (prev != NULL && prev->is_free) {
        prev->size += sizeof(Node) + selected->size;
        remove_node(selected);
        // point selected at prev for the next if block
        selected = prev;
    }

    if (next != NULL && next->is_free) {
        selected->size += sizeof(Node) + next->size;
        remove_node(next);
    }
}

/* If ltail is a free Node, sets ltail to ltail's previous,
 * removes the previous ltail from the list, and sets the break
 * to the start of the address that was just removed. If there was
 * only one Node in the list, then lhead is also reset to NULL.
 */
void shrink_brk() {
    if (ltail->is_free) {
        Node *old_end = ltail;
        ltail = ltail->prev;

        if (ltail == NULL) {
            // then we've just freed the last node
            lhead = NULL;
        } else {
            ltail->next = NULL;
        }

        brk(old_end);
        // the below doesn't work after running qsort
        // I suspect the implementation thereof might allocate somehow?
        // sbrk(-(old_end->size + sizeof(Node)));
        // UPDATE: I seem to be losing brk to qsort at TIMES >= 256
    }
}

/* Inserts the Node "first" immediately after the Node "second"
 * in first's linked list. if first = ltail, updates ltail
 */
void insert_after(Node *first, Node *second) {
    assert(first != NULL);
    if (second != NULL) {
        second->next = first->next;
        second->prev = first;
        if (first == ltail) {
            ltail = second;
        } else {
            first->next->prev = second;
        }
    }
    first->next = second;
}

/* Removes the given Node from the list, connecting its prev
 * and next Nodes and/or updating lhead and ltail
 * as appropriate.
 */
void remove_node(Node *to_remove) {
    Node *prev = to_remove->prev;
    Node *next = to_remove->next;

    if (prev != NULL) {
        prev->next = next;
    } else {
        lhead = next;
    }

    if (next != NULL) {
        next->prev = prev;
    } else {
        ltail = prev;
    }
}
