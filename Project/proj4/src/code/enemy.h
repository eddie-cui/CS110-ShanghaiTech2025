#ifndef ENEMY_H
#define ENEMY_H

#include "GameObject.h"
#include "lcd/lcd.h"
#include "bullets.h"

void Enemy_Show(GameObject* enemy, int color);
GameObject* Enemy_add(GameObject* head,int x, int y, enum type enemy_type);
int Enemy_rigidBody(GameObject* enemy, int x, int y);
void Enemy_Shooting(GameObject* enemy, int target_x, int target_y);
void Enemy_Update(GameObject* enemy);
void Enemy_Delete(GameObject* enemy);

#endif // ENEMY_H