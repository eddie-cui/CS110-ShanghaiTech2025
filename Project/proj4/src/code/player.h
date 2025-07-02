#ifndef PLAYER_H
#define PLAYER_H
#include "GameObject.h"
#include "bullets.h"
struct coordinate {
    int x; // x coordinate
    int y; // y coordinate
};
enum direction {UP, DOWN, LEFT, RIGHT, NONE};
void Player_Show(GameObject* player, int color);
GameObject* Player_Init(int x, int y);
void Player_Update(GameObject* player,enum direction dir);
void Player_Shooting(GameObject* player);
struct coordinate Player_Get_Coordinate(GameObject* player);
#endif
