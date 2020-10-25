/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2009 Altera Corporation, San Jose, California, USA.           *
* All rights reserved.                                                        *
*                                                                             *
* Permission is hereby granted, free of charge, to any person obtaining a     *
* copy of this software and associated documentation files (the "Software"),  *
* to deal in the Software without restriction, including without limitation   *
* the rights to use, copy, modify, merge, publish, distribute, sublicense,    *
* and/or sell copies of the Software, and to permit persons to whom the       *
* Software is furnished to do so, subject to the following conditions:        *
*                                                                             *
* The above copyright notice and this permission notice shall be included in  *
* all copies or substantial portions of the Software.                         *
*                                                                             *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     *
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         *
* DEALINGS IN THE SOFTWARE.                                                   *
*                                                                             *
* This agreement shall be governed in all respects by the laws of the State   *
* of California and by the laws of the United States of America.              *
*                                                                             *
******************************************************************************/

#include <fcntl.h>
#include <string.h>

#include "sys/alt_dev.h"
#include "sys/alt_irq.h"
#include "sys/ioctl.h"
#include "sys/alt_errno.h"

#include "altera_avalon_uart.h"
#include "altera_avalon_uart_regs.h"

/* The Rx/Tx buffer shared with our application.
 * Size : WL_PKT_MAXLEN bytes.
 *
 * Since communication protocol just consists in answering
 * after being asked, there's no need to separate send
 * and receive buffers.
 */
extern unsigned char _link_rxtx_buffer[];


/* ----------------------------------------------------------- */
/* ------------------------- FAST DRIVER --------------------- */
/* ----------------------------------------------------------- */

/*
 * altera_avalon_uart_init() is called by the auto-generated function 
 * alt_sys_init() in order to initialize a particular instance of this device.
 * It is responsible for configuring the device and associated software 
 * constructs.
 */

#ifdef ALT_ENHANCED_INTERRUPT_API_PRESENT
static void altera_avalon_uart_irq(void* context);
#else
static void altera_avalon_uart_irq(void* context, alt_u32 id);
#endif 

void 
altera_avalon_uart_init(altera_avalon_uart_state* sp, 
  alt_u32 irq_controller_id,  alt_u32 irq)
{
  void* base = sp->base;
  int error;

  /* 
   * Initialise the read and write flags and the semaphores used to 
   * protect access to the circular buffers when running in a multi-threaded
   * environment.
   */
  error = ALT_FLAG_CREATE (&sp->events, 0)    || 
          ALT_SEM_CREATE (&sp->read_lock, 1)  ||
          ALT_SEM_CREATE (&sp->write_lock, 1);

  if (!error)
  {
    /* Initialize and set pointers to Rx/Tx buffers. */
    memset(_link_rxtx_buffer, 0, WL_PKT_MAXLEN);
    sp->rx_buf = _link_rxtx_buffer;
    sp->tx_buf = _link_rxtx_buffer;

    /* enable interrupts at the device */
    sp->ctrl = ALTERA_AVALON_UART_CONTROL_RTS_MSK  |
                ALTERA_AVALON_UART_CONTROL_RRDY_MSK |
                ALTERA_AVALON_UART_CONTROL_DCTS_MSK;

    IOWR_ALTERA_AVALON_UART_CONTROL(base, sp->ctrl); 
  
    /* register the interrupt handler */
#ifdef ALT_ENHANCED_INTERRUPT_API_PRESENT
    alt_ic_isr_register(irq_controller_id, irq, altera_avalon_uart_irq, sp, 
      0x0);
#else
    alt_irq_register (irq, sp, altera_avalon_uart_irq);
#endif  
  }
}

/*
 * altera_avalon_uart_irq() is the interrupt handler registered at 
 * configuration time for processing UART interrupts. It processes
 * interrupt requests to either incoming data and/or outgoing data.
 */
#ifdef ALT_ENHANCED_INTERRUPT_API_PRESENT
static void altera_avalon_uart_irq(void* context)
#else
static void altera_avalon_uart_irq(void* context, alt_u32 id)
#endif
{
    alt_u32 status;

    altera_avalon_uart_state* sp = (altera_avalon_uart_state*) context;
    void* base                   = sp->base;

    /*
     * Read the status register in order to determine the cause of the
     * interrupt.
     */
    status = IORD_ALTERA_AVALON_UART_STATUS(base);

    /* Clear any error flags set at the device */
    IOWR_ALTERA_AVALON_UART_STATUS(base, 0);

    /* Dummy read to ensure IRQ is negated before ISR returns */
    IORD_ALTERA_AVALON_UART_STATUS(base);
  
    /*
     * Process a receive interrupt. It transfers the incoming character
     * into the receive buffer, and sets the apropriate flags to indicate
     * that there is data ready to be processed.
     */
    if(status & ALTERA_AVALON_UART_STATUS_RRDY_MSK)
    {
        if(status & (ALTERA_AVALON_UART_STATUS_PE_MSK | ALTERA_AVALON_UART_STATUS_FE_MSK))
        {
            /* If there was an error, discard the data */
        }
        else
        {
            /* Retrieve byte received from UART to buffer. */
            sp->rx_buf[sp->rx_count] = IORD_ALTERA_AVALON_UART_RXDATA(sp->base);
            sp->rx_count++;

            /*
             * If the cicular buffer was full, disable interrupts. Interrupts will be
             * re-enabled when data is removed from the buffer.
             */
            if(sp->rx_count > ALT_AVALON_UART_BUF_LEN)
            {
                sp->ctrl &= ~ALTERA_AVALON_UART_CONTROL_RRDY_MSK;
                IOWR_ALTERA_AVALON_UART_CONTROL(sp->base, sp->ctrl); 
            }
        }
    }

    /*
     * Process a transmit interrupt. It transfers data from the transmit 
     * buffer to the device, and sets the apropriate flags to indicate that 
     * there is data ready to be processed.
     */
    if(status & (ALTERA_AVALON_UART_STATUS_TRDY_MSK | ALTERA_AVALON_UART_STATUS_DCTS_MSK))
    {
        /* Transfer data if there is some ready to be transfered */
        if(sp->tx_pos < sp->tx_count)
        {
            /* Write the data to the device */
            IOWR_ALTERA_AVALON_UART_TXDATA(sp->base, sp->tx_buf[sp->tx_pos]);
            sp->tx_pos++;
        }

        if(sp->tx_pos >= sp->tx_count)
        {
            /*
             * If the circular buffer is empty, disable the interrupt. This will be
             * re-enabled when new data is placed in the buffer.
             */
            sp->ctrl &= ~(ALTERA_AVALON_UART_CONTROL_TRDY_MSK | ALTERA_AVALON_UART_CONTROL_DCTS_MSK);
        }
        else
        {
            /*
             * In case the tranmit interrupt had previously been disabled by 
             * detecting a low value on CTS, it is reenabled here.
             */ 
            sp->ctrl |= ALTERA_AVALON_UART_CONTROL_TRDY_MSK;
        }

        IOWR_ALTERA_AVALON_UART_CONTROL(sp->base, sp->ctrl);
    }
}


/*
 * The close() routine is implemented to drain the UART transmit buffer
 * when not in "small" mode. This routine will wait for transimt data to be
 * emptied unless the driver flags have been set to non-blocking mode. 
 * This routine should be called indirectly (i.e. though the C library 
 * close() routine) so that the file descriptor associated with the relevant 
 * stream (i.e. stdout) can be closed as well. This routine does not manage 
 * file descriptors.
 * 
 * The close routine is not implemented for the small driver; instead it will
 * map to null. This is because the small driver simply waits while characters
 * are transmitted; there is no interrupt-serviced buffer to empty 
 */
int altera_avalon_uart_close(altera_avalon_uart_state* sp, int flags)
{
//  /* 
//   * Wait for all transmit data to be emptied by the UART ISR.
//   */
//  while (sp->tx_start != sp->tx_end) {
//    if (flags & O_NONBLOCK) {
//      return -EWOULDBLOCK; 
//    }
//  }

  return 0;
}

