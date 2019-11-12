#include <stdio.h>
#include <unistd.h>
#include <assert.h>

#define BREAK_INVARIANT list_end == NULL \
    || ((void *) list_end + list_end->size + sizeof(Node) == sbrk(0))
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
Node *allocate_at_break(int);
Node *firstfit_find(int);
void insert_after(Node *, Node *);
void remove_node(Node *);
Node *list_start = NULL;
Node *list_end = NULL;

// TODO: missing space??
void *my_firstfit_malloc(int size) {
    static int times_called = 0;
    times_called++;
    Node *selected;
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
            insert_after(list_end, selected);
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
        insert_after(selected, new);
    }
}

/*
 * inserts the Node "first" immediately after the Node "second"
 * in first's linked list. if first is the list tail, updates the tail
 */
void insert_after(Node *first, Node *second) {
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

void remove_node(Node *to_remove) {
    Node *prev = to_remove->prev;
    Node *next = to_remove->next;

    if (prev != NULL) {
        prev->next = next;
    } else {
        list_start = next;
    }

    if (next != NULL) {
        next->prev = prev;
    } else {
        list_end = prev;
    }
}

/*
 * TODO
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
    Node *to_free = location;
    called++;
    to_free -= 1;
    assert(!to_free->is_free);

    to_free->is_free = TRUE;
    assert(BREAK_INVARIANT);
    coalesce(to_free);
    assert(BREAK_INVARIANT);
    shrink_brk();
    assert(BREAK_INVARIANT);
}

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

/*
 * shrink the brk if the last node is free.
 */
void shrink_brk() {
    if (list_end->is_free) {
        Node *old_end = list_end;
        list_end = list_end->prev;

        // shrink brk
        sbrk(-(old_end->size + sizeof(Node)));

        if (list_end == NULL) {
            // then we've just freed the last node
            list_start = NULL;
        } else {
            list_end->next = NULL;
        }
    }
}

