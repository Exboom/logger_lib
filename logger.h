#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#define LOG_PRINTF_BUFF_SIZE     64 // to change
#define LOG_PRINTF_TIME_SIZE     18 // 01.01.02 00:00:00 + " " formate
#define FORMAT_FLAG_LEFT_JUSTIFY (1u << 0)
#define FORMAT_FLAG_PAD_ZERO     (1u << 1)
#define FORMAT_FLAG_PRINT_SIGN   (1u << 2)
#define FORMAT_FLAG_ALTERNATE    (1u << 3)
#define TO_SD                    0 // buff_index mark
#define TO_FLASH                 1 // buff_index mark

void reg_logger_func(char* (*rtc_func)(void), uint8_t (*sd_func)(const char *buffer), uint8_t (flash_func)(const char *buffer));

typedef struct {
    char *    get_time;
    char *    write_str;
    unsigned  max_str_len;
    int       return_val;
    unsigned  cnt;
    unsigned  buff_index;

} LOG_PRINTF_DESC;

int log_printf(unsigned buff_index, const char * sFormat, ...); //buff_index - index SD or flash
