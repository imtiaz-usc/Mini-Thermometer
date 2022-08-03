void rot_init(void);

extern volatile unsigned char a, b, new_state, old_state, changed;
extern volatile char input_bit, low_thresh, low_temp, high_thresh, high_temp;
extern volatile int count, low, high, mode;
extern volatile unsigned int temperature;
