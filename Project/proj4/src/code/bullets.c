#include "bullets.h"
#include "lcd/lcd.h"
#include <stdlib.h>

extern int bullet_count;
void Bullets_Show(GameObject* bullet,int color){
    if (bullet == NULL) {
        return;
    }

    
    if(color!=-1){
        if(bullet->object_type == bullet_1) {
            LCD_DrawRectangle(bullet->x-1, bullet->y+1,bullet->x+1 ,bullet->y-1, color);
        } else if(bullet->object_type == bullet_2) {
            LCD_DrawLine(bullet->x-1, bullet->y, bullet->x+1, bullet->y, color);
        } else if(bullet->object_type == bullet_3) {
            LCD_DrawPoint(bullet->x, bullet->y, color);
        }
        else if(bullet->object_type == bullet_player1||bullet->object_type == bullet_player2||bullet->object_type == bullet_player3|| bullet->object_type == bullet_player4) {
            LCD_DrawPoint(bullet->x, bullet->y, color);
        }
        return;
    }
    if(bullet->object_type == bullet_1) {
            LCD_DrawRectangle(bullet->x-1, bullet->y+1,bullet->x+1 ,bullet->y-1, YELLOW);
    } else if(bullet->object_type == bullet_2) {
            LCD_DrawLine(bullet->x-1, bullet->y, bullet->x+1, bullet->y, RED);
    } else if(bullet->object_type == bullet_3) {
        LCD_DrawPoint(bullet->x, bullet->y, GREEN);
    }
    else if(bullet->object_type == bullet_player1||bullet->object_type == bullet_player2||bullet->object_type == bullet_player3|| bullet->object_type == bullet_player4) {
        LCD_DrawPoint(bullet->x, bullet->y, WHITE);
    }
}
void Bullets_Create(GameObject* head, int x, int y, enum type bullet_type,int target_x, int target_y) {
    GameObject* bullet = GameObject_Add(head, x, y, bullet_type,target_x,target_y);
    bullet_count++;
    if (bullet == NULL) {
        return;
    }
    if(bullet_type == bullet_1) {
        Bullets_Show(bullet, YELLOW); 
    } else if(bullet_type == bullet_2) {
        Bullets_Show(bullet, RED); 
    } else if(bullet_type == bullet_3) {
        Bullets_Show(bullet, GREEN);
    }
    else if(bullet_type == bullet_player1||bullet_type == bullet_player2||bullet_type == bullet_player3||bullet_type == bullet_player4) {
        Bullets_Show(bullet, WHITE);
    }
    if(bullet->object_type==bullet_player3){
        bullet->target_x = 0;
        bullet->target_y = y;
    }
}
void Bullets_Delete(GameObject* bullet) {
    if (bullet == NULL) {
        return; 
    }
    
    bullet_count--; 
    GameObject_delete(bullet);
}


void Bullets_Update(GameObject* bullet,int new_x, int new_y) {
    if(bullet->x < 0 || bullet->x > 160 || bullet->y < 0 || bullet->y > 80) {
        Bullets_Show(bullet, BLACK); 
        Bullets_Delete(bullet);
        return;
    }
    if(bullet->object_type==bullet_player1){
        Bullets_Show(bullet, BLACK);
        bullet->y--;
        Bullets_Show(bullet, WHITE);
        return;

    }
    if(bullet->object_type==bullet_player2){
        Bullets_Show(bullet, BLACK);
        int baisy=bullet->target_y;
        int dy=baisy-bullet->y; 
        if(dy>=20){
            bullet->target_y = bullet->y;
            dy=0;
        }
        if(dy<5){
            bullet->y--;
            bullet->x++;
        }
        if(dy>=5&&dy<15){
            bullet->y--;
            bullet->x--;
        }
        if(dy>=15&&dy<20){
            bullet->y--;
            bullet->x++;
        }
        Bullets_Show(bullet, WHITE);
        return;
    }
    if(bullet->object_type==bullet_player3){
        Bullets_Show(bullet, BLACK);
        

        int horizontal_step = 1;
        int vertical_step = 1;
        

        int total_step= bullet->target_x;
        int stage=total_step/4;
        stage%=8;
        if(stage==0){
            bullet->x+=horizontal_step;
        }else if(stage==1){
            bullet->y-=vertical_step;
        }else if(stage==2){
            bullet->y-=vertical_step;
        }else if(stage==3){
            bullet->x-=vertical_step;
        }
        else if(stage==4){
            bullet->x-=horizontal_step;
        }else if(stage==5){
            bullet->y-=vertical_step;
        }else if(stage==6){
            bullet->y-=vertical_step;
        }else if(stage==7){
            bullet->x+=vertical_step; 
        }
        Bullets_Show(bullet, WHITE);
        bullet->target_x ++; 
        return;
    }
    if(bullet->object_type==bullet_player4){
        Bullets_Show(bullet, BLACK);
        int dx=new_x - bullet->x;
        int dy=new_y - bullet->y;
        bullet->x += dx>0?2:-2;
        bullet->y += dy>0?2:-2;
        Bullets_Show(bullet, WHITE);
        return;
    }
    Bullets_Show(bullet, BLACK);

    int dx=bullet->target_x - bullet->x; 
    int dy=bullet->target_y - bullet->y;
    bullet->x += dx>0?2:-2;
    bullet->y += dy>0?-2:2;
    Bullets_Show(bullet, -1);

}