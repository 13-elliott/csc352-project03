#include <stdio.h>
#include <unistd.h>
#include <assert.h>

#define TRUE 1
#define FALSE 0

typedef int bool;

typedef struct Node {
    int size;
    bool is_free;
    struct Node *prev;
    struct Node *next;
} Node;

void *my_firstfit_malloc(int);

void my_free(void *);

void shrink_brk();

void coalesce(Node *);

bool is_node_in_list(Node *);

void allocate_at_node(Node *, int);

void connect_after(Node *, Node *);

Node *allocate_at_break(int);

Node *firstfit_find(int);

Node *list_start = NULL;
Node *list_end = NULL;

void *initial = NULL;

#define BREAK_INVARIANT list_end == NULL || (void *) list_end + list_end->size + sizeof(Node) == sbrk(0)

// TODO: missing space??
void *my_firstfit_malloc(int size) {
    static int times_called = 0;
    times_called++;
    Node *selected;
    if (initial == NULL) {
        initial = sbrk(0);
    }
    if (list_start == NULL) {
        // Make first allocation and return the
        // the address immediately after the Node
        selected = allocate_at_break(size);
        selected->prev = selected->next = NULL;
        list_start = list_end = selected;
    } else {
        selected = firstfit_find(size);
        if (selected == NULL) {
            selected = allocate_at_break(size);
            connect_after(list_end, selected);
        } else {
            allocate_at_node(selected, size);
        }
    }
    assert(BREAK_INVARIANT);
    return selected + 1;
}

// TODO: document
void allocate_at_node(Node *selected, int request_size) {
    assert(selected != NULL);
    assert(selected->is_free);
    assert(selected->size >= request_size);

    // leftover space in this region (after another Node would be added)
    int leftover_space = selected->size - request_size - sizeof(Node);

    selected->is_free = FALSE;
    if (leftover_space > 0) {
        // create new node at beginning of the leftover space
        Node *new = (void *) (selected + 1) + request_size;
        selected->size = request_size;
        new->is_free = TRUE;
        new->size = leftover_space;
        connect_after(selected, new);
    }
}

/*
 * inserts the Node "latter" immediately after the Node "prior"
 * in prior's linked list
 */
void connect_after(Node *first, Node *second) {
    assert(first != NULL);
    if (second != NULL) {
        second->next = first->next;
        second->prev = first;
        if (first == list_end) {
            list_end = second;
        } else {
            first->next->prev = second;
        }
    }
    first->next = second;

}

/*
 * initializes the first node of the linked list.
 * init is the initial address of brk as returned by sbrk(n);
 */
Node *allocate_at_break(int request_size) {
    Node *new = sbrk(sizeof(Node) + request_size);
    new->is_free = FALSE;
    new->size = request_size;
    return new;
}

/* returns the node denoting the first space in which the requested allocation
 * can be fit. returns NULL if no fit can be found.
 */
Node *firstfit_find(int size) {
    Node *curr = list_start;
    while (curr != NULL) {
        if (curr->is_free && curr->size > size) {
            // exits loop to return stmt while curr refers
            // to the desired node
            break;
        }
        curr = curr->next;
    }
    return curr;
}

void my_free(void *location) {
    static int called = 0;
    called++;
    // assert(location != NULL);
    Node *to_free = location;
    to_free -= 1;
    assert(!to_free->is_free);

    to_free->is_free = TRUE;
    coalesce(to_free);
    shrink_brk();
    assert(BREAK_INVARIANT);
}

void coalesce(Node *selected) {
    return; // TODO: FIX ME!?
    Node *prev = selected->prev;
    if (prev != NULL && prev->is_free) {
        prev->size += sizeof(Node) + selected->size;
        connect_after(prev, selected->next);
        selected = prev;
    }
    // because the end of the list should never be a free node,
    // selected->next should not be the end of the list
    // and selected->next->next should not be NULL
    Node *next = selected->next;
    if (next != NULL && next->is_free) {
        selected->size += sizeof(Node) + next->size;
        connect_after(selected, next->next);
        if (next == list_end) {
            list_end = selected;
        }
    }
}

/*
 * shrink the brk if the last node is free.
 */
void shrink_brk() {
    // keep shrinking the break
    // until the last node isn't free
    // (or there are no more nodes)
    assert(BREAK_INVARIANT);
    while (list_end != NULL && list_end->is_free) {
        Node *old_end = list_end;
        list_end = list_end->prev;

        //assert(old_end == NULL || (void *) old_end + sizeof(Node) + old_end->size == sbrk(0));
        // shrink brk
        sbrk(-(old_end->size + sizeof(Node)));
        if (list_end == NULL) {
            // then we've just freed the last node
            list_start = NULL;
            break;
        } else {
            list_end->next = NULL;
        }
    }
}

// for assertions
bool is_node_in_list(Node *nd) {
    Node *curr = list_start;
    while (curr != NULL) {
        if (curr == nd) {
            return TRUE;
        }
        curr = curr->next;
    }
    return FALSE;
}

