#ifndef KEYBOARD_H
#define KEYBOARD_H

int kbhit();
void nonblock( int state );

extern int s_interrupted;

void s_signal_handler (int signal_value);
 
void s_catch_sigint_signal (void);


#endif // KEYBOARD_H
