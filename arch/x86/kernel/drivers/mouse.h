#ifndef MOUSE_H
#define MOUSE_H

typedef struct {
    int x;
    int y;
    int buttons;  /* bit 0 = left, bit 1 = right, bit 2 = middle */
} mouse_state_t;

void mouse_init(void);
void mouse_handler(void);
void mouse_get_state(mouse_state_t* out);
int  mouse_get_x(void);
int  mouse_get_y(void);
int  mouse_get_buttons(void);

#endif
