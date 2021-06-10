#include "logger.h"

/* Heandlers */
char* (*rtc_time_get)();
uint8_t (*sd_push)(const char *buffer);
uint8_t (*flash_push)(const char *buffer);

/* Registration functions */
void reg_logger_func(char* (*rtc_func)(void), uint8_t (*sd_func)(const char *buffer), uint8_t (flash_func)(const char *buffer)) {
    rtc_time_get = rtc_func;   // get time function
    sd_push      = sd_func;    // push to sd function
    flash_push   = flash_func; // push to flash function
}

/* Write to flash or SD */
unsigned log_write(unsigned buff_index, const char * buffer) {
    unsigned status;

    switch (buff_index) {
    case TO_SD:
        if (sd_push(buffer) == 0) {
            status = 0u;
        } else
            status = 1u;
        break;

    case TO_FLASH:
        if (flash_push(buffer) == 0) {
            status = 0u;
        } else
            status = 1u;
        break;

    default:
        status = 1u;
        break;
    }

    return status;
}

static void _StoreChar(LOG_PRINTF_DESC *p, char c) {
    unsigned Cnt;

    /* Filling the buffer */
    Cnt = p->cnt;
    if ((Cnt + 1u) <= p->max_str_len) {
        *(p->write_str + Cnt) = c;
        p->cnt                = Cnt + 1u;
        p->return_val++;
    }
    /* Write when the buffer is full */
    if (p->cnt == p->max_str_len) {
        char full_str[LOG_PRINTF_BUFF_SIZE] = {};
        strcpy(full_str, p->get_time);
        strcat(full_str, p->write_str); // compound time and message
        if (log_write(p->buff_index, full_str) != 0u) {
            p->return_val = -1;
        }
    }
}

static void _PrintUnsigned(LOG_PRINTF_DESC *buffer_desc, unsigned v, unsigned base, unsigned num_digits, unsigned field_width, unsigned format_flags) {
    static const char _aV2C[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                   '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    unsigned          div;
    unsigned          digit;
    unsigned          number;
    unsigned          width;
    char              c;

    number = v;
    digit  = 1u;

    width = 1u;
    while (number >= base) {
        number = (number / base);
        width++;
    }
    if (num_digits > width) {
        width = num_digits;
    }

    if ((format_flags & FORMAT_FLAG_LEFT_JUSTIFY) == 0u) {
        if (field_width != 0u) {
            if (((format_flags & FORMAT_FLAG_PAD_ZERO) == FORMAT_FLAG_PAD_ZERO) && (num_digits == 0u)) {
                c = '0';
            } else {
                c = ' ';
            }
            while ((field_width != 0u) && (width < field_width)) {
                field_width--;
                _StoreChar(buffer_desc, c);
                if (buffer_desc->return_val < 0) {
                    break;
                }
            }
        }
    }

    if (buffer_desc->return_val >= 0) {
        while (1) {
            if (num_digits > 1u) {
                num_digits--;
            } else {
                div = v / digit;
                if (div < base) {
                    break;
                }
            }
            digit *= base;
        }
        do {
            div = v / digit;
            v -= div * digit;
            _StoreChar(buffer_desc, _aV2C[div]);
            if (buffer_desc->return_val < 0) {
                break;
            }
            digit /= base;
        } while (digit);
        if ((format_flags & FORMAT_FLAG_LEFT_JUSTIFY) == FORMAT_FLAG_LEFT_JUSTIFY) {
            if (field_width != 0u) {
                while ((field_width != 0u) && (width < field_width)) {
                    field_width--;
                    _StoreChar(buffer_desc, ' ');
                    if (buffer_desc->return_val < 0) {
                        break;
                    }
                }
            }
        }
    }
}

static void _PrintInt(LOG_PRINTF_DESC * buffer_desc, int v, unsigned base, unsigned num_digits, unsigned field_width, unsigned format_flags) {
    unsigned Width;
    int      Number;

    Number = (v < 0) ? -v : v;
    Width  = 1u;
    while (Number >= (int) base) {
        Number = (Number / (int) base);
        Width++;
    }
    if (num_digits > Width) {
        Width = num_digits;
    }
    if ((field_width > 0u) &&
        ((v < 0) || ((format_flags & FORMAT_FLAG_PRINT_SIGN) == FORMAT_FLAG_PRINT_SIGN))) {
        field_width--;
    }
    if ((((format_flags & FORMAT_FLAG_PAD_ZERO) == 0u) || (num_digits != 0u)) &&
        ((format_flags & FORMAT_FLAG_LEFT_JUSTIFY) == 0u)) {
        if (field_width != 0u) {
            while ((field_width != 0u) && (Width < field_width)) {
                field_width--;
                _StoreChar(buffer_desc, ' ');
                if (buffer_desc->return_val < 0) {
                    break;
                }
            }
        }
    }
    if (buffer_desc->return_val >= 0) {
        if (v < 0) {
            v = -v;
            _StoreChar(buffer_desc, '-');
        } else if ((format_flags & FORMAT_FLAG_PRINT_SIGN) == FORMAT_FLAG_PRINT_SIGN) {
            _StoreChar(buffer_desc, '+');
        } else {
        }
        if (buffer_desc->return_val >= 0) {
            if (((format_flags & FORMAT_FLAG_PAD_ZERO) == FORMAT_FLAG_PAD_ZERO) &&
                ((format_flags & FORMAT_FLAG_LEFT_JUSTIFY) == 0u) && (num_digits == 0u)) {
                if (field_width != 0u) {
                    while ((field_width != 0u) && (Width < field_width)) {
                        field_width--;
                        _StoreChar(buffer_desc, '0');
                        if (buffer_desc->return_val < 0) {
                            break;
                        }
                    }
                }
            }
            if (buffer_desc->return_val >= 0) {
                _PrintUnsigned(buffer_desc, (unsigned) v, base, num_digits, field_width, format_flags);
            }
        }
    }
}

int log_vprintf(unsigned buff_index, const char * sFormat, va_list * param_list) {
    char            c;
    LOG_PRINTF_DESC buff_log;
    int             v;
    unsigned        num_digits;
    unsigned        format_flags;
    unsigned        field_width;
    char            msgBuff[LOG_PRINTF_BUFF_SIZE - LOG_PRINTF_TIME_SIZE] = {}; // message without a timestamp

    buff_log.write_str   = msgBuff;
    buff_log.max_str_len = LOG_PRINTF_BUFF_SIZE - LOG_PRINTF_TIME_SIZE; // len message without a timestamp
    buff_log.cnt         = 0u;
    buff_log.buff_index  = buff_index; 
    buff_log.return_val  = 0;
    buff_log.get_time    = rtc_time_get();

    do {
        c = *sFormat;
        sFormat++;
        if (c == 0u) {
            break;
        }
        if (c == '%') {
            format_flags = 0u;
            v            = 1;
            do {
                c = *sFormat;
                switch (c) {
                case '-':
                    format_flags |= FORMAT_FLAG_LEFT_JUSTIFY;
                    sFormat++;
                    break;
                case '0':
                    format_flags |= FORMAT_FLAG_PAD_ZERO;
                    sFormat++;
                    break;
                case '+':
                    format_flags |= FORMAT_FLAG_PRINT_SIGN;
                    sFormat++;
                    break;
                case '#':
                    format_flags |= FORMAT_FLAG_ALTERNATE;
                    sFormat++;
                    break;
                default:
                    v = 0;
                    break;
                }
            } while (v);
            field_width = 0u;
            do {
                c = *sFormat;
                if ((c < '0') || (c > '9')) {
                    break;
                }
                sFormat++;
                field_width = (field_width * 10u) + ((unsigned) c - '0');
            } while (1);
            num_digits = 0u;
            c          = *sFormat;
            if (c == '.') {
                sFormat++;
                do {
                    c = *sFormat;
                    if ((c < '0') || (c > '9')) {
                        break;
                    }
                    sFormat++;
                    num_digits = num_digits * 10u + ((unsigned) c - '0');
                } while (1);
            }
            c = *sFormat;
            do {
                if ((c == '1') || (c == 'h')) {
                    sFormat++;
                    c = *sFormat;
                } else {
                    break;
                }
            } while (1);
            switch (c) {
            case 'c': {
                char c0;
                v  = va_arg(*param_list, int);
                c0 = (char) v;
                _StoreChar(&buff_log, c0);
                break;
            }
            case 'd':
                v = va_arg(*param_list, int);
                _PrintInt(&buff_log, v, 10u, num_digits, field_width, format_flags);
                break;
            case 'u':
                v = va_arg(*param_list, int);
                _PrintUnsigned(&buff_log, (unsigned) v, 10u, num_digits, field_width, format_flags);
                break;
            case 'x':
                v = va_arg(*param_list, int);
                _PrintUnsigned(&buff_log, (unsigned) v, 16u, num_digits, field_width, format_flags);
                break;
            case 's': {
                const char *s = va_arg(*param_list, const char *);
                do {
                    c = *s;
                    s++;
                    if (c == '\0') {
                        break;
                    }
                    _StoreChar(&buff_log, c);
                } while (buff_log.return_val >= 0);
            } break;
            case 'p':
                v = va_arg(*param_list, int);
                _PrintUnsigned(&buff_log, (unsigned) v, 16u, 8u, 8u, 0u);
                break;
            case '%':
                _StoreChar(&buff_log, '%');
                break;
            default:
                break;
            }
            sFormat++;
        } else {
            _StoreChar(&buff_log, c);
        }
    } while (buff_log.return_val >= 0);
    if (buff_log.return_val > 0) {
        if (buff_log.cnt != 0u) {
            char full_str[LOG_PRINTF_BUFF_SIZE] = {};
            strcpy(full_str, buff_log.get_time);
            strcat(full_str, msgBuff); // compound time and message
            log_write(buff_index, full_str);
        }
        buff_log.return_val += (int) buff_log.cnt;
    }
    return buff_log.return_val;
}

int log_printf(unsigned buff_index, const char * sFormat, ...) {
    int     r;
    va_list param_list;
    va_start(param_list, sFormat);
    r = log_vprintf(buff_index, sFormat, &param_list);
    va_end(param_list);
    return r;
}