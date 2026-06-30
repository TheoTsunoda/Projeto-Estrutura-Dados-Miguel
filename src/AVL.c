#include "AVL.h"
#include "alloc.h"
#include <string.h>


typedef struct AVLNode {
    Title *t;
    struct AVLNode *left;
    struct AVLNode *right;
    int height;
} AVLNode;

typedef struct {
    AVLNode *root;
    size_t   count;
    size_t   bytes;
} AVLTree;

static int get_height(AVLNode *n) {
    return n ? n->height : 0;
}

static int max_val(int a, int b) {
    return (a > b) ? a : b;
}

static int get_balance(AVLNode *n) {
    return n ? get_height(n->left) - get_height(n->right) : 0;
}


static AVLNode *right_rotate(AVLNode *y) {
    AVLNode *x = y->left;
    AVLNode *T2 = x->right;


    x->right = y;
    y->left = T2;

    y->height = max_val(get_height(y->left), get_height(y->right)) + 1;
    x->height = max_val(get_height(x->left), get_height(x->right)) + 1;

    return x;
}


static AVLNode *left_rotate(AVLNode *x) {
    AVLNode *y = x->right;
    AVLNode *T2 = y->left;

    y->left = x;
    x->right = T2;

    x->height = max_val(get_height(x->left), get_height(x->right)) + 1;
    y->height = max_val(get_height(y->left), get_height(y->right)) + 1;

    return y;
}


static AVLNode *avl_insert_node(AVLTree *T, AVLNode *node, Title *t, int *updated) {
    if (!node) {
        AVLNode *new_node = (AVLNode *)xmalloc(sizeof(AVLNode));
        if (!new_node) return NULL;
        new_node->t = t;
        new_node->left = NULL;
        new_node->right = NULL;
        new_node->height = 1;

        T->count++;
        T->bytes += sizeof(AVLNode);
        return new_node;
    }

    int cmp = strcmp(t->tconst, node->t->tconst);

    if (cmp < 0) {
        node->left = avl_insert_node(T, node->left, t, updated);
    } else if (cmp > 0) {
        node->right = avl_insert_node(T, node->right, t, updated);
    } else {
        node->t = t;
        *updated = 1;
        return node;
    }

    if (*updated) return node;

    node->height = 1 + max_val(get_height(node->left), get_height(node->right));

    int balance = get_balance(node);

    if (balance > 1 && strcmp(t->tconst, node->left->t->tconst) < 0)
        return right_rotate(node);

    if (balance < -1 && strcmp(t->tconst, node->right->t->tconst) > 0)
        return left_rotate(node);

    if (balance > 1 && strcmp(t->tconst, node->left->t->tconst) > 0) {
        node->left = left_rotate(node->left);
        return right_rotate(node);
    }

    if (balance < -1 && strcmp(t->tconst, node->right->t->tconst) < 0) {
        node->right = right_rotate(node->right);
        return left_rotate(node);
    }

    return node;
}

static AVLNode *min_value_node(AVLNode *node) {
    AVLNode *current = node;
    while (current->left != NULL)
        current = current->left;
    return current;
}

static AVLNode *avl_remove_node(AVLTree *T, AVLNode *root, const char *key, int *removed) {
    if (!root) return root;

    int cmp = strcmp(key, root->t->tconst);

    if (cmp < 0) {
        root->left = avl_remove_node(T, root->left, key, removed);
    } else if (cmp > 0) {
        root->right = avl_remove_node(T, root->right, key, removed);
    } else {
        *removed = 1;

        if (!root->left || !root->right) {
            AVLNode *child = root->left ? root->left : root->right;
            xfree(root);
            T->count--;
            T->bytes -= sizeof(AVLNode);
            return child;
        } else {
            AVLNode *temp = min_value_node(root->right);
            root->t = temp->t;
            root->right = avl_remove_node(T, root->right, temp->t->tconst, removed);
        }
    }

    if (!root) return root;

    root->height = 1 + max_val(get_height(root->left), get_height(root->right));

    int balance = get_balance(root);


    if (balance > 1 && get_balance(root->left) >= 0)
        return right_rotate(root);

    if (balance > 1 && get_balance(root->left) < 0) {
        root->left = left_rotate(root->left);
        return right_rotate(root);
    }

    if (balance < -1 && get_balance(root->right) <= 0)
        return left_rotate(root);

    if (balance < -1 && get_balance(root->right) > 0) {
        root->right = right_rotate(root->right);
        return left_rotate(root);
    }

    return root;
}


static void destroy_nodes(AVLNode *node) {
    if (!node) return;
    destroy_nodes(node->left);
    destroy_nodes(node->right);
    xfree(node);
}



static void avl_insert(void *self, Title *t) {
    AVLTree *T = (AVLTree *)self;
    int updated = 0;
    T->root = avl_insert_node(T, T->root, t, &updated);
}

static Title *avl_search(void *self, const char *key) {
    AVLTree *T = (AVLTree *)self;
    AVLNode *current = T->root;

    while (current) {
        int cmp = strcmp(key, current->t->tconst);
        if (cmp == 0)  return current->t;
        if (cmp < 0)   current = current->left;
        else           current = current->right;
    }
    return NULL;
}

static int avl_remove(void *self, const char *key) {
    AVLTree *T = (AVLTree *)self;
    int removed = 0;
    T->root = avl_remove_node(T, T->root, key, &removed);
    return removed;
}

static size_t avl_size(void *self) {
    return ((AVLTree *)self)->count;
}

static size_t avl_mem_bytes(void *self) {
    return ((AVLTree *)self)->bytes;
}

static void avl_destroy(void *self) {
    AVLTree *T = (AVLTree *)self;
    destroy_nodes(T->root);
    xfree(T);
}



Dictionary AVL_create(void) {
    AVLTree *T = (AVLTree *)xmalloc(sizeof(AVLTree));
    T->root = NULL;
    T->count = 0;
    T->bytes = sizeof(AVLTree);

    Dictionary d;
    d.self      = T;
    d.name      = "AVL Tree";
    d.insert    = avl_insert;
    d.search    = avl_search;
    d.remove    = avl_remove;
    d.size      = avl_size;
    d.mem_bytes = avl_mem_bytes;
    d.destroy   = avl_destroy;
    return d;
}