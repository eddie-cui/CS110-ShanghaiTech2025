#include "player.h"
#include "lcd/lcd.h"
extern int shooting_type;
void Player_Show(GameObject* player, int color){
    if (player == NULL) {
        return;
    }
    if(color!=-1){
        LCD_DrawPoint_big(player->x, player->y, color); 
        return;
    }
    LCD_DrawPoint_big(player->x, player->y, WHITE); 
}
GameObject* Player_Init(int x, int y){
    GameObject* player = GameObject_Init(x, y, Player);
    if (player == NULL) {
        return NULL;
    }
    Player_Show(player, WHITE);
    return player;
}
void Player_Update(GameObject* player,enum direction dir){
    if (player == NULL) {
        return;
    }
    Player_Show(player, BLACK);
    switch (dir) {
        case UP:
            player->y-=2;
            break;
        case DOWN:
            player->y+=2;
            break;
        case LEFT:
            player->x-=2;
            break;
        case RIGHT:
            player->x+=2;
            break;
        case NONE:

            break;
    }
    if(player->x < 0) player->x = 0;
    if(player->x > 160) player->x = 160;
    if(player->y < 0) player->y = 0;
    if(player->y > 80) player->y = 80;
    Player_Show(player, WHITE);
}
void Player_Shooting(GameObject* player){
    if (player == NULL) {
        return; 
    }

    if(shooting_type==1){
        Bullets_Create(player, player->x, player->y, bullet_player1, player->x, player->y);
    } else if(shooting_type==2){
        Bullets_Create(player, player->x, player->y, bullet_player2, player->x, player->y);
    } else if(shooting_type==3){
        Bullets_Create(player, player->x, player->y, bullet_player3, player->x, player->y);
    }
    else if(shooting_type==4){
        Bullets_Create(player, player->x, player->y, bullet_player4, player->x, player->y);
    }
}
struct coordinate Player_Get_Coordinate(GameObject* player){
    struct coordinate pos;
    if (player == NULL) {
        pos.x = -1; 
        pos.y = -1;
        return pos;
    }
    pos.x = player->x;
    pos.y = player->y;
    return pos;
}