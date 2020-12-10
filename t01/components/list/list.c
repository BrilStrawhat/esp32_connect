#include "list.h"

t_list *mx_create_node(void *data) {
    t_list *node = (t_list *)malloc(sizeof(t_list));

    node->data = data;
    node->next = NULL;
    return node;
}

void mx_push_front(t_list **list, void *data) {
    t_list *new = mx_create_node(data);

    new->next = *list;
    *list = new;
}

void mx_push_back(t_list **list, void *data) {
    t_list *new = mx_create_node(data);
    t_list *last = *list;

    if (*list == NULL) {
        *list = new;
        return;
    }
    while (last->next)
        last = last->next;
    last->next = new;
}

void mx_pop_front(t_list **head) {
    t_list *p = NULL;

    if (!head || !(*head))
        return;
    if ((*head)->next == NULL) {
        free((*head)->data);
        free(*head);
        *head = NULL;
    }
    else {
        p = (*head)->next;
        free((*head)->data);
        free(*head);
        *head = p;
    }
}

void mx_pop_back(t_list **head) {
    t_list *p = NULL;

    if (!head || !(*head))
        return;
    if (!(*head)->next) {
        free((*head)->data);
        free(*head);
        *head = NULL;
        return;
    }
    else {
        p = *head;
        while (p->next->next)
            p = p->next;
        free(p->next->data);
        free(p->next);
        p->next = NULL;
    }
}

void mx_list_foreach(t_list *head, void (funct)(void*)) {
    while (head != NULL) {
        funct(head->data);
        head = head->next;
    }
}

int mx_list_size(t_list *list) {
    int count = 0;
    t_list *current = list;

    while (current) {
        count++;
        current = current->next;
    }
    return count;
}
