#include <stdio.h>
#include <unistd.h>
#include <assert.h>

typedef struct Node {
    int size;
    int is_free; // bool
    struct Node *prev;
    struct Node *next;
} Node;

void *my_firstfit_malloc(int);
void my_free(void *);
Node *expand_allocate(int);
void allocate_at(Node *, int);
void connect_after(Node *, Node *);
Node *make_first_node(void *);
Node *firstfit_find(int);

Node *list_start = NULL;
Node *list_end = NULL;

void *my_firstfit_malloc(int size) {
    if (list_start == NULL) {
        printf("allocating first node\n");
        Node *first = sbrk(size + sizeof(Node));
        make_first_node(first);
        return first + 1;
    } else {
        printf("allocating other nodes\n");
        Node *selected = firstfit_find(size);
        if (selected == NULL) {
            Node *new = expand_allocate(size);
            return new + 1;
        } else {
            allocate_at(selected, size);
            return selected + 1;
        }
    }
    
    return NULL; // temp
}

// expands the heap and creates a new node where brk was. 
    
Node *expand_allocate(int size) {
    assert(size > 0);
    Node *new = sbrk(size + sizeof(Node));
    new->size = size;
    new->is_free = 0; // false
    // prev & next of new are set in connect_after
    connect_after(list_end, new);
    list_end = new;
    return new;
}

// TODO: document
void allocate_at(Node *selected, int request_size) {
    assert(selected != NULL);
    assert(selected->is_free);
    assert(selected->size >= request_size);
    
    Node *next = selected->next;
    Node *prev = selected->prev;
    // leftover space in this region (after another Node would be added)
    int leftover_space = selected->size - request_size - sizeof(Node);
    
    selected->is_free = 0;
    if (leftover_space > 0) {
        Node *new = (void*) (selected + 1) + request_size;
        selected->size = request_size;
        new->is_free = 1; // true
        new->size = leftover_space;
        connect_after(selected, new);
        if (list_end == selected) {
            list_end = new;
        }
    }
}

/*
 * inserts the Node "latter" immediately after the Node "prior"
 * in prior's linked list
 */
void connect_after(Node *prior, Node *latter) {
    latter->next = prior->next;
    latter->prev = prior;
    prior->next = latter;
}

/*
 * initializes the first node of the linked list.
 * init is the initial address of brk as returned by sbreak(n);
 */
Node *make_first_node(void *init) {
    assert(list_start == NULL);
    assert(list_end == NULL);
    Node *new = list_start = list_end = init;
    new->prev = NULL;
    new->next = NULL;
    new->is_free = 0; // false
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
    Node *current = location - sizeof(Node);
    // TODO
}
