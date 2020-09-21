#include "log.h"
#include "link.h"

#include "wasca_defs.h"


/*
 * ------------------------------------------
 * - Log stuff ------------------------------
 * ------------------------------------------
 */


#if LOG_ENABLE == 1
/* Circular debugger global definition. */
log_infos_t* _log_inf_ptr = NULL;
#endif // LOG_ENABLE == 1


void log_init(void)
{
#if LOG_ENABLE == 1
    /* Initialize pointer to log informations structure on SDRAM. */
    _log_inf_ptr = &(wasca_internals->log_infos);

    /* Initialize log internals themselves. */
    log_infos_t* logs = _log_inf_ptr;
    memset(logs, 0, sizeof(log_infos_t));

    /* Reset circular buffer pointers. */
    logs->readptr  = 0;
    logs->writeptr = logs->readptr;

    /* Set internals. */
    logs->start_address = (unsigned long)logs->circbuff;
    logs->end_address   = logs->start_address + sizeof(logs->circbuff) - 1;
    logs->buffer_size   = sizeof(logs->circbuff);
#endif // LOG_ENABLE == 1
}

#if LOG_ENABLE == 1

/**
 * Write type+datalen+data in Circular buffer.
 * It modifies circular buffer's write pointer.
**/
void log_cbwrite(unsigned char* data, unsigned long datalen)
{
    log_infos_t* logs = _log_inf_ptr;

//memset((unsigned char*)EXTERNAL_SDRAM_CONTROLLER_BASE, 0x00, 256);
//memcpy((unsigned char*)EXTERNAL_SDRAM_CONTROLLER_BASE, data, datalen);

    if(logs->writeptr >= logs->readptr)
    {
        /* Enough free space ? */
        if((datalen + 1) > (logs->buffer_size - logs->writeptr + logs->readptr - 1))
        {
            return;
        }

        unsigned long end_space = logs->buffer_size - logs->writeptr;

        if(end_space >= datalen)
        { /* Enough space until the end of the buffer. */
            memcpy((void*)(logs->start_address+logs->writeptr), 
                   (void*)(data), 
                   datalen);
        }
        else
        { /* Need to write the data in 2 times: end of buffer, and the remaining from the buffer start */
            memcpy((void*)(logs->start_address+logs->writeptr), 
                   (void*)(data), 
                   end_space);

            memcpy((void*)(logs->start_address), 
                   (void*)(data+end_space), 
                   datalen - end_space);
        }
    }
    else
    {
        /* Enough free space ? */
        if((datalen + 1) > (logs->readptr - logs->writeptr - 1))
        {
            return;
        }

        memcpy((void*)(logs->start_address+logs->writeptr), 
               (void*)(data), 
               datalen);
    }

    /* Update write pointer. */
    unsigned long ptr = logs->writeptr + datalen;
    logs->writeptr = (ptr >= logs->buffer_size ? ptr - logs->buffer_size : ptr);
}


// /**
//  * Return circular buffer memory usage.
// **/
// unsigned long log_cbmem_use(void)
// {//return 0; // TMP
//     log_infos_t* logs = _log_inf_ptr;
//     unsigned long ret;
// 
//     if(logs->writeptr == logs->readptr)
//     {
//         ret = 0;
//     }
//     else if(logs->writeptr >= logs->readptr)
//     {
//         ret = logs->writeptr - logs->readptr + 1;
//     }
//     else //!if(d->writeptr >= d->readptr)
//     {
//         ret = logs->buffer_size - logs->readptr + logs->writeptr + 1;
//     }
// 
//     return ret;
// }


// /**
//  * Read specified length from logs circular buffer.
//  * If specified, circular buffer read pointer is updated.
//  *
//  * Note : it won't behave correctly if specified read length
//  *        is higher than used length in circular buffer.
// **/
// void log_cbread(unsigned char* data, unsigned long read_len, int update_rp)
// {
//     log_infos_t* logs = _log_inf_ptr;
// 
//     if(read_len == 0)
//     {
//         /* Nothing to read ? */
//         return;
//     }
// 
//     if(logs->writeptr > logs->readptr)
//     {
//         memcpy((void*)(data), 
//                (void*)(logs->start_address+logs->readptr), 
//                read_len);
//     }
//     else
//     {
//         unsigned long first_len = logs->buffer_size - logs->readptr;
//         memcpy((void*)(data), 
//                (void*)(logs->start_address+logs->readptr), 
//                (read_len > first_len ? first_len : read_len));
// 
//         if(read_len > first_len)
//         {
//             memcpy((void*)(data + first_len), 
//                    (void*)(logs->start_address), 
//                    read_len - first_len);
//         }
//     }
// 
//     /* If requested, update read pointer. */
//     if(update_rp)
//     {
//         unsigned long ptr = logs->readptr + read_len;
//         logs->readptr = (ptr >= logs->buffer_size ? ptr - logs->buffer_size : ptr);
//     }
// }

#endif // LOG_ENABLE == 1


void logout_internal(int level, const char* fmt, ... )
{
#if LOG_ENABLE == 1
    log_infos_t* logs = _log_inf_ptr;

    va_list args;
    va_start(args, fmt);
    const char *w;
    char c;

    /* Buffer where to format log message.
     * First two bytes are used to store message length.
     */
    unsigned short len = 0;
    char* buff = (char*)(logs->format_buff + sizeof(len));

    /* Process format string. */
    w = fmt;
    while ((c = *w++) != 0)
    {
        /* If not a format escape character, just print  */
        /* character.  Otherwise, process format string. */
        if (c != '%')
        {
            buff[len++] = c;
        }
        else
        {
            /* Get format character.  If none     */
            /* available, processing is complete. */
            if ((c = *w++) != 0)
            {
                if (c == '%')
                {
                    /* Process "%" escape sequence. */
                    buff[len++] = c;
                } 
                else if (c == 'c')
                {
                    int v = va_arg(args, int);
                    buff[len++] = v;
                }
                else if (c == 's')
                {
                    /* Process string format. */
                    char *s = va_arg(args, char *);

                    while(*s)
                    {
                        buff[len++] = *s++;
                    }
                }
                else
                {
                    /* Process hexadecimal number format.
                     * If not '%x' format, provide format and value in
                     * output string.
                     * Example : "%u[CAFE]", which should be displayed as
                     *           "51966" on PC side.
                     */
                    int direct_print = ((c == 'x') || (c == 'X') ? 1 : 0);
                    unsigned long v = va_arg(args, unsigned long);
                    unsigned long digit;
                    int digit_shift;

                    /* If not direct print, provide format in output buffer. */
                    if(!direct_print)
                    {
                        buff[len++] = '%';
                        while(1)
                        {
                            buff[len++] = c;

                            /* Don't forget to provide any eventual
                             * length specificator.
                             */
                            if((c < '0') || (c > '9'))
                            {
                                break;
                            }

                            c = *w++;
                        }

                        /* Add delimiter for value. */
                        buff[len++] = '[';
                    }

                    if (v == 0)
                    {
                        /* If the number value is zero, just print and continue. */
                        buff[len++] = '0';
                    }
                    else
                    {
                        /* Find first non-zero digit. */
                        digit_shift = 28;
                        while (!(v & (0xF << digit_shift)))
                        {
                            digit_shift -= 4;
                        }

                        /* Print digits. */
                        for (; digit_shift >= 0; digit_shift -= 4)
                        {
                            digit = (v & (0xF << digit_shift)) >> digit_shift;
                            if (digit <= 9)
                            {
                                c = '0' + digit;
                            }
                            else
                            {
                                c = 'A' + digit - 10;
                            }
                            buff[len++] = c;
                        }
                    }

                    /* If not direct print, don't forget to close value delimiter. */
                    if(!direct_print)
                    {
                        buff[len++] = ']';
                    }
                }
            }
            else
            {
                break;
            }
        }
    }

    /* Append to circular buffer.
     * First two bytes are used to store message length.
     * Then, log message without terminating null character is stored.
     */
    len += sizeof(len);
    memcpy(logs->format_buff, &len, sizeof(len));
    log_cbwrite(logs->format_buff, len);
#endif // LOG_ENABLE == 1
}


void logflush_internal(void)
{
#if LOG_ENABLE == 1
    log_infos_t* logs = _log_inf_ptr;

    /* Wait until logs read ptr == write ptr. */
    do
    {
        link_do_io();
    }while(logs->writeptr != logs->readptr);
#endif // LOG_ENABLE == 1
}

