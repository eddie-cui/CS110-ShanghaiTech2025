#include "button_lock.h"
struct button_state {
    int wait_time; //50hz so wait for 15times
    int locked;  // 1 if the button is locked, 0 otherwise
}buttons[7];
void Button_Init(void) {
    buttons[JOY_UP_BTN] = (struct button_state){0, 0};
    buttons[JOY_DOWN_BTN] = (struct button_state){0, 0};
    buttons[JOY_LEFT_BTN] = (struct button_state){0, 0};
    buttons[JOY_RIGHT_BTN] = (struct button_state){0, 0};
    buttons[JOY_CTR_BTN] = (struct button_state){0, 0};
    buttons[BUTTON_1_BTN] = (struct button_state){0, 0};
    buttons[BUTTON_2_BTN] = (struct button_state){0, 0};
}
void Lock_Button(enum button_type btn) {
    struct button_state *button = &buttons[btn];
    if (button->locked) {
        return;
    }
    button->locked = 1;
    button->wait_time = 15;


}

void Update_Button() {
    for (int i = 0; i < 7; i++) {
        if (buttons[i].locked) {
            buttons[i].wait_time--;
            if (buttons[i].wait_time == 0) {
                buttons[i].locked = 0;
            }
        }
    }
}
int Query_Button_locked(enum button_type btn) {
    struct button_state *button = &buttons[btn];
    if (button->locked) {
        return 1;
    }

    return 0;
}
