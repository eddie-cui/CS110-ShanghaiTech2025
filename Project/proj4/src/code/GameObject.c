#include"GameObject.h"
// #define NULL ((void*)0)
GameObject* GameObject_Init(int x, int y, enum type object_type) {
    GameObject* head = (GameObject*)malloc(sizeof(GameObject));
    if (head == NULL) {
        return NULL;
    }
    head->x = x; 
    head->y = y;
    head->target_x = 0; 
    head->target_y = 0;
    head->object_type = object_type;
    head->next = NULL; 
    head->prev = NULL;
    return head;
}
GameObject* GameObject_Add(GameObject* head, int x, int y, enum type object_type,
                           int target_x, int target_y) {
    GameObject* new_object = (GameObject*)malloc(sizeof(GameObject));
    if (new_object == NULL) {
        return NULL;
    }
    new_object->x = x;
    new_object->y = y;
    new_object->target_x = target_x;
    new_object->target_y = target_y; 
    new_object->object_type = object_type;
    new_object->next=head->next;
    new_object->prev = head;
    if (head->next != NULL) {
        head->next->prev = new_object;
    }
    head->next = new_object;
    return new_object;
}
void GameObject_delete(GameObject* obj) {
    if (obj == NULL) {
        return;
    }
    if (obj->prev != NULL) {
        obj->prev->next = obj->next;
    }
    if (obj->next != NULL) {
        obj->next->prev = obj->prev;
    }
    free(obj);
}

void GameObject_delete_all(GameObject* head) {
    GameObject* current = head;
    while (current != NULL) {
        GameObject* next = current->next;
        GameObject_delete(current);
        current = next;
    }
}
