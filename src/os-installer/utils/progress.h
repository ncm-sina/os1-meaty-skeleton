#ifndef PROGRESS_H
#define PROGRESS_H

void init_progress(uint32_t max_value, uint8_t width);
void update_progress(uint32_t value);
uint32_t get_progress(void);

#endif
