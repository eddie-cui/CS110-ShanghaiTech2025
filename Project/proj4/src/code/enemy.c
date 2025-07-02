#include "enemy.h"
#include <stdlib.h>
int Enemy_Random_Direction() {
    return rand() % 4;

}
void Enemy_Show(GameObject* enemy, int color) {
    if (enemy == NULL) {
        return;
    }
    if(color != -1) {
        if(enemy->object_type == enemy_1) {
            LCD_DrawPoint_big(enemy->x, enemy->y, color); 
        } else if(enemy->object_type == enemy_2) {
            LCD_DrawPoint_big(enemy->x, enemy->y, color); 
        } else if(enemy->object_type == enemy_3) {
            LCD_DrawPoint_big(enemy->x, enemy->y, color); 
        }
        return;
    }
    else{
        if(enemy->object_type == enemy_1) {
            LCD_DrawPoint_big(enemy->x, enemy->y, YELLOW); 
        } else if(enemy->object_type == enemy_2) {
            LCD_DrawPoint_big(enemy->x, enemy->y, RED); 
        } else if(enemy->object_type == enemy_3) {
            LCD_DrawPoint_big(enemy->x, enemy->y, GREEN); 
        }
        return;
    }
}
GameObject* Enemy_add(GameObject* head,int x, int y, enum type enemy_type) {
    GameObject* enemy = GameObject_Add(head,x, y, enemy_type,0,0);
    if (enemy == NULL) {
        return NULL;
    }
    Enemy_Show(enemy, -1);
    return enemy;
}
int Enemy_rigidBody(GameObject* enemy, int x, int y) {
    if (enemy == NULL) {
        return 0; 
    }
    if (x>= enemy->x - 1 && x <= enemy->x + 1 &&
        y >= enemy->y - 1 && y <= enemy->y + 1) {
        return 1; 
    }

    return 0; 
}
void Enemy_Shooting(GameObject* enemy, int target_x, int target_y) {
    if (enemy == NULL) {
        return;
    }
    if(enemy->object_type == enemy_1) {
        Bullets_Create(enemy, enemy->x, enemy->y, bullet_1, enemy->x, -2);
    } else if(enemy->object_type == enemy_2) {
        Bullets_Create(enemy, enemy->x, enemy->y, bullet_2, enemy->x, -2);
    } else if(enemy->object_type == enemy_3) {
        Bullets_Create(enemy, enemy->x, enemy->y, bullet_3, enemy->x, -2);
        Bullets_Create(enemy, enemy->x, enemy->y, bullet_3, enemy->x, 82);
        Bullets_Create(enemy, enemy->x, enemy->y, bullet_3, -2, enemy->y);
        Bullets_Create(enemy, enemy->x, enemy->y, bullet_3, 162, enemy->y);
        Bullets_Create(enemy, enemy->x, enemy->y, bullet_3, enemy->x+180, enemy->y+180);
        Bullets_Create(enemy, enemy->x, enemy->y, bullet_3, enemy->x+180, enemy->y-180);
        Bullets_Create(enemy, enemy->x, enemy->y, bullet_3, enemy->x-180, enemy->y+180);
        Bullets_Create(enemy, enemy->x, enemy->y, bullet_3, enemy->x-180, enemy->y-180);
    }
}
void Enemy_Update(GameObject* enemy) {
    if (enemy == NULL) {
        return;
    }
    Enemy_Show(enemy, BLACK); 
    int dir = Enemy_Random_Direction();
    switch (dir) {
        case 1:
            enemy->y--;
            break;
        case 2:
            enemy->y++;
            break;
        case 3:
            enemy->x--;
            break;
        case 0:
            enemy->x++;
            break;
        default:
            break;
    }
    if(enemy->x < 0) enemy->x = 0; 
    if(enemy->x > 160) enemy->x = 160; 
    if(enemy->y < 0) enemy->y = 0; 
    if(enemy->y > 30) enemy->y = 30; 
    Enemy_Show(enemy, -1); 
}
void Enemy_Delete(GameObject* enemy) {
    if (enemy == NULL) {
        return; 
    }
    Enemy_Show(enemy, BLACK);
    GameObject_delete(enemy);
}
