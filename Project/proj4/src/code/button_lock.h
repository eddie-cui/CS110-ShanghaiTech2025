#ifndef BUTTON_LOCK_H
#define BUTTON_LOCK_H
enum button_type{
    JOY_UP_BTN,
    JOY_DOWN_BTN,
    JOY_LEFT_BTN,
    JOY_RIGHT_BTN,
    JOY_CTR_BTN,
    BUTTON_1_BTN,
    BUTTON_2_BTN
};
void Button_Init(void);
void Lock_Button(enum button_type btn);
void Update_Button(void);
int Query_Button_locked(enum button_type btn);


#endif // BUTTON_LOCK_H
