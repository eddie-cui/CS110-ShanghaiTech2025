#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H
#include <stdlib.h>
enum type{Player,enemy_1,enemy_2,enemy_3,bullet_1,bullet_2,bullet_3,bullet_player1,bullet_player2,bullet_player3,bullet_player4};
typedef struct GameObject GameObject;


struct GameObject {
    int x;               
    int y;              
    enum type object_type;  
    int target_x;       
    int target_y;    
    GameObject* next; 
    GameObject* prev;
};
GameObject* GameObject_Init(int x, int y, enum type object_type);
GameObject* GameObject_Add(GameObject* head, int x, int y, enum type object_type,
                           int target_x, int target_y);
void GameObject_delete(GameObject* obj);
void GameObject_delete_all(GameObject* head);

#endif
