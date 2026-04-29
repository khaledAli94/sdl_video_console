#ifndef INPUT_RAW_H
#define INPUT_RAW_H

void input_raw_init(void);
void input_raw_shutdown(void);
int input_raw_read(unsigned char *buf, int max);

#endif
