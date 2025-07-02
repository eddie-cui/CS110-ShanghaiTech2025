#ifndef BULLETS_H
#define BULLETS_H
#include "GameObject.h"
void Bullets_Show(GameObject* bullet,int color);
void Bullets_Create(GameObject* head,int x, int y, enum type bullet_type,
                    int target_x, int target_y);
void Bullets_Delete(GameObject* bullet);
void Bullets_Update(GameObject* bullet,int new_x, int new_y);


#endif
