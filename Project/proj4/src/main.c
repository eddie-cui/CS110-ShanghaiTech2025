#include "lcd/lcd.h"
#include "utils.h"
#include "assembly/level_choose.h"
#include "code/button_lock.h"
#include "code/GameObject.h"
#include "code/player.h"
#include "code/bullets.h"
#include "code/enemy.h"

#include "gd32vf103.h"
#include "systick.h"
char Level_1[] = "Level 1";
char Level_2[] = "Level 2";
char Level_3[] = "Level 3";
int Current_selection = 3;
int bullet_count = 0; 
int shooting_type=1; 
int random_num(int num) {
    return rand() % num;
}
int create_bullet(int level){
  switch (level) {
    case 1:
      return random_num(10)<1;
    case 2:
      return random_num(10)<2;
    case 3:
      return 1;
    default:
      return 0;
  }
}
void Inp_init(void) {
  rcu_periph_clock_enable(RCU_GPIOA);
  rcu_periph_clock_enable(RCU_GPIOC);

  gpio_init(GPIOA, GPIO_MODE_IPD, GPIO_OSPEED_50MHZ,
            GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
  gpio_init(GPIOC, GPIO_MODE_IPD, GPIO_OSPEED_50MHZ,
            GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);
}

void IO_init(void) {
  Inp_init(); 
  Lcd_Init(); 
  Button_Init();
  LCD_Clear(BLACK); 
}
int frame_old=0;
int Selecting(void) {
  CreateLevelChoose();
  // while(1){
  //   Level_Update();
  //   Update_Button(); 
  //   if(Get_Button(JOY_DOWN)&& !Query_Button_locked(JOY_DOWN_BTN)) {
  //     Current_selection++;
  //     if(Current_selection > 5) {
  //       Current_selection = 3; 
  //     }
  //     Lock_Button(JOY_DOWN_BTN); 
  //   }
  //   if(Get_Button(JOY_UP)&& !Query_Button_locked(JOY_UP_BTN)) {
  //     Current_selection--;

  //     if(Current_selection < 1) {
  //       Current_selection = 3;
  //     }
  //     Lock_Button(JOY_UP_BTN);
  //   }
  //   if(Get_Button(JOY_CTR)&& !Query_Button_locked(JOY_CTR_BTN)) {
  //     LCD_Clear(BLACK);
  //     u16 Level=Press_Button(Current_selection);
  //     Lock_Button(JOY_CTR_BTN);
  //     return Level;
  //   }
  //   delay_1ms(10);
  // }
  return Selecting_ASM();
    
}

void Playing(int level){
  int total_bullets_created = 0;
  bullet_count = 0; 
  int frame=0;
  LCD_Clear(BLACK); 

  uint64_t start_time,end_time;
  start_time = get_timer_value();
  uint64_t tmp;
  do{
      tmp = get_timer_value();
  }while(tmp == start_time);
  GameObject* Head = Player_Init(80, 40); 
  if (Head == NULL) {
    return;
  }
  struct coordinate enemy_pos[3];
  GameObject* enemy1 = Enemy_add(Head, random_num(160), random_num(30), enemy_1);
  GameObject* enemy2 = Enemy_add(Head, random_num(160), random_num(30), enemy_2);
  GameObject* enemy3 = Enemy_add(Head, random_num(160), random_num(30), enemy_3);
  if (enemy1 == NULL || enemy2 == NULL || enemy3 == NULL) {
    return; 
  }
  enemy_pos[0].x = enemy1->x;
  enemy_pos[0].y = enemy1->y; 
  enemy_pos[1].x = enemy2->x; 
  enemy_pos[1].y = enemy2->y;
  enemy_pos[2].x = enemy3->x;
  enemy_pos[2].y = enemy3->y; 

  int pending_bullet_count = 0;
  GameObject* pending_enemy;
  uint64_t fps_start_time = start_time;
  int frame_count = 0;
  int current_fps = 0;
  uint64_t bullet_timer_start = start_time;
  int bullets_created_start = 0;
  int bullets_created_per_sec = 0; 
  int total_f=0;
      int wait_time=0;
  int bullet_total=0;
  while(1){
    uint64_t frame_start = get_timer_value();
    total_f++;
    bullet_total+= bullet_count;
        frame_count++;
        uint64_t elapsed_time = frame_start - fps_start_time;
        uint64_t elapsed_ms = elapsed_time / (SystemCoreClock / 4000);

        if(elapsed_ms >= 120) {

            // LCD_Fill(0, 0, 80, 15, BLACK);
            // LCD_Fill(120, 0, 160, 20, BLACK);

            current_fps = frame_count; 
            LCD_ShowString(5, 5, (u8*)"FPS:", WHITE);
            LCD_ShowNum(35, 5, current_fps*1000/120, 3, WHITE);
            // LCD_Fill(120, 0, 160, 20, BLACK);
            LCD_ShowNum(120, 5, bullet_total/frame_count, 3, WHITE);
            bullet_total = 0;

            frame_count = 0;
            fps_start_time = frame_start;
        }
    enum direction dir = NONE;
    if(pending_bullet_count > 0) {
      pending_bullet_count--; 
      Enemy_Shooting(pending_enemy,Head->x , Head->y);
    }
    if(pending_bullet_count ==0){
      wait_time++;
    }
    Update_Button(); 
    if(Get_Button(JOY_UP)&& !Query_Button_locked(JOY_UP_BTN)) {
      dir = UP;
      Lock_Button(JOY_UP_BTN); 
    }
    if(Get_Button(JOY_DOWN)&& !Query_Button_locked(JOY_DOWN_BTN)) {
      dir = DOWN;
      Lock_Button(JOY_DOWN_BTN); 
    }
    if(Get_Button(JOY_LEFT)&& !Query_Button_locked(JOY_LEFT_BTN)) {
      dir = LEFT;
      Lock_Button(JOY_LEFT_BTN);
    }
    if(Get_Button(JOY_RIGHT)&& !Query_Button_locked(JOY_RIGHT_BTN)) {
      dir = RIGHT;
      Lock_Button(JOY_RIGHT_BTN);
    }
    if(Get_Button(BUTTON_2)&& !Query_Button_locked(BUTTON_2_BTN)) {
      shooting_type++;
      if(shooting_type > 4) {
        shooting_type = 1;
      }
      Lock_Button(BUTTON_2_BTN);
    }
    if(Get_Button(BUTTON_1)&& !Query_Button_locked(BUTTON_1_BTN)) {
      Player_Shooting(Head);
      Lock_Button(BUTTON_1_BTN);
    }
    if(Get_Button(JOY_CTR)&& !Query_Button_locked(JOY_CTR_BTN)) {
      LCD_Clear(BLACK);
      GameObject_delete_all(Head);
      Lock_Button(JOY_CTR_BTN);
      delay_1ms(100);
      return; 
    }
    Player_Update(Head, dir);
    GameObject* object = Head->next;

while (object != NULL) {
    GameObject* temp = object->next; 
    int object_deleted = 0; 

    if(object->object_type == bullet_player1 || object->object_type == bullet_player2 || object->object_type == bullet_player3|| object->object_type == bullet_player4) {
        if(Enemy_rigidBody(enemy1, object->x, object->y)==1) {
            Enemy_Delete(enemy1); 
            Bullets_Show(object, BLACK); 
            Bullets_Delete(object);
            enemy1 = Enemy_add(Head, random_num(160), random_num(80), enemy_1);
            if (enemy1 == NULL) {
                return; 
            }
            enemy_pos[0].x = enemy1->x; 
            enemy_pos[0].y = enemy1->y;
            object_deleted = 1;
        }
        else if(Enemy_rigidBody(enemy2, object->x, object->y)==1) {
            Enemy_Delete(enemy2);
            Bullets_Show(object, BLACK);
            Bullets_Delete(object);
            enemy2 = Enemy_add(Head, random_num(160), random_num(80), enemy_2);
            pending_bullet_count = 0; 
            if (enemy2 == NULL) {
                return;
            }
            enemy_pos[1].x = enemy2->x;
            enemy_pos[1].y = enemy2->y; 
            object_deleted = 1;
        }
        else if(Enemy_rigidBody(enemy3, object->x, object->y)==1) {
            Enemy_Delete(enemy3);
            Bullets_Show(object, BLACK);
            Bullets_Delete(object); 
            enemy3 = Enemy_add(Head, random_num(160), random_num(80), enemy_3); 
            if (enemy3 == NULL) {
                return; 
            }
            enemy_pos[2].x = enemy3->x; 
            enemy_pos[2].y = enemy3->y; 
            object_deleted = 1;
        }
    }
    

    
    if (!object_deleted) {
        struct coordinate pos = Player_Get_Coordinate(Head); 
        if(object->object_type == bullet_player4){
          int closest_enemy_index = -1;
          int closest_distance = 10000;
          for(int i = 0; i < 3; i++) {
              int distance = abs(enemy_pos[i].x - pos.x) + abs(enemy_pos[i].y - pos.y);
              if(distance < closest_distance) {
                  closest_distance = distance;
                  closest_enemy_index = i;
              }
          }
          Bullets_Update(object, enemy_pos[closest_enemy_index].x, enemy_pos[closest_enemy_index].y);
        }
        if(object->object_type == bullet_player1 || object->object_type == bullet_player2 || object->object_type == bullet_player3) {
            Bullets_Update(object, object->x, object->y);
        }
        if(object->object_type == bullet_1 || object->object_type == bullet_2 || object->object_type == bullet_3) {
            if(level!=3)
                Bullets_Update(object, 0, 0);
            else
                Bullets_Update(object, pos.x, pos.y); 
        }
        if(object->object_type == enemy_1 || object->object_type == enemy_2 || object->object_type == enemy_3) {
            if(level==3){
              if(object->object_type==enemy_1){
                if(bullet_count<=270)
                {
                  Bullets_Create(object, object->x, object->y, bullet_3, object->x, -2);
                }
                Enemy_Update(object); 
                enemy_pos[0].x = object->x; 
                enemy_pos[0].y = object->y;
              }
              if(object->object_type==enemy_2){
                if(bullet_count<=270&&wait_time>10)
                {
                  Enemy_Shooting(object, pos.x, pos.y);
                      pending_bullet_count+=20;
                      wait_time=0;
                      pending_enemy = object;
                }
                Enemy_Update(object); 
                enemy_pos[1].x = object->x;
                enemy_pos[1].y = object->y;
              }
              if(object->object_type==enemy_3){
                if(bullet_count<=270)
                {
                  Bullets_Create(Head, object->x, object->y, bullet_3, object->x, -2);
                  Bullets_Create(Head, object->x, object->y, bullet_3, object->x, 82);
                  Bullets_Create(Head, object->x, object->y, bullet_3, -2, object->y);
                  Bullets_Create(Head, object->x, object->y, bullet_3, 162, object->y);
                  Bullets_Create(Head, object->x, object->y, bullet_3, object->x+180, object->y+180);
                  Bullets_Create(Head, object->x, object->y, bullet_3, object->x+180, object->y-180);
                  Bullets_Create(Head, object->x, object->y, bullet_3, object->x-180, object->y+180);
                  Bullets_Create(Head, object->x, object->y, bullet_3, object->x-180, object->y-180);
                }
                Enemy_Update(object);
                enemy_pos[2].x = object->x; 
                enemy_pos[2].y = object->y;
              }
            }
            else
            {
              int check_shooting= create_bullet(level); 
              if(object->object_type==enemy_1){
                  if(check_shooting!=0&&bullet_count<=270){
                      Enemy_Shooting(object, pos.x, pos.y); 
                  }
                  Enemy_Update(object);
                  enemy_pos[0].x = object->x; 
                  enemy_pos[0].y = object->y;
              }
              if(object->object_type==enemy_2){
                  if(pending_bullet_count==0&&check_shooting!=0&&bullet_count<=270&&wait_time>10) 
                  {
                      Enemy_Shooting(object, pos.x, pos.y); 
                      pending_bullet_count+=20;
                      wait_time=0;
                      pending_enemy = object;
                  }
                  Enemy_Update(object);
                  enemy_pos[1].x = object->x;
                  enemy_pos[1].y = object->y; 
              }
              if(object->object_type==enemy_3){
                  if(check_shooting!=0&&bullet_count<=270){
                      Enemy_Shooting(object, pos.x, pos.y);
                  }
                  Enemy_Update(object);
                  enemy_pos[2].x = object->x;
                  enemy_pos[2].y = object->y;
              }
            }
        }
    }
    object = temp;
  }
  if(bullet_count<=270){
    if(level==3&& total_f%20==0){
      int ran= random_num(160);
      Bullets_Create(Head, ran, 1, bullet_1, ran, -2);
    }
  }
  end_time = get_timer_value();
        uint64_t frame_time = end_time - frame_start;
        uint64_t target_frame_time = SystemCoreClock / 4000 * 20;
        
        if (frame_time < target_frame_time) {
            uint64_t delay_time = target_frame_time - frame_time;
            uint32_t delay_ms = delay_time / (SystemCoreClock / 4000);
            if(delay_ms > 0 && delay_ms < 50) {
                delay_1ms(delay_ms);
            }
        }
      }
}
int main(void) {
  IO_init();
  while(1){
    u16 Level = Selecting();
    Playing(Level);
  }
}
