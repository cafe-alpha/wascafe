/**
 *  Cartridge Test Utility
 *  by cafe-alpha.
 *  WWW: http://ppcenter.free.fr/pskai/
 *
 *  See LICENSE file for details.
**/

//                                                                                
//    Copyright (c) 2006 Thomas Fuchs / The Rockin'-B, www.rockin-b.de       
//                                                                                
//    Permission is granted to anyone to use this software for any purpose        
//    and to redistribute it freely, subject to the following restrictions:       
//                                                                                
//    1.  The origin of this software must not be misrepresented. You must not    
//        claim that you wrote the original software. If you use this software    
//        in a product, an acknowledgment in the product documentation would be   
//        appreciated but is not required.                                        
//                                                                                
//    2.  ANY COMMERCIAL USE IS PROHIBITED. This includes that you must not use   
//        this software in the production of a commercial product. Only an explicit
//        permission by the author can allow you to do that.                      
//                                                                                
//    3.  Any modification applied to the software must be marked as such clearly.
//                                                                                
//    4.  This license may not be removed or altered from any distribution.       
//                                                                                
//                                                                                
//    			    NO WARRANTY                                                 
//                                                                                
//    THERE IS NO WARRANTY FOR THE PROGRAM.                                       
//    THE PROGRAM IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,               
//    EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO,                 
//    THE IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR PURPOSE.                 
//    THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU.
//    SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY    
//    SERVICING, REPAIR OR CORRECTION.                                            
//                                                                                
//    IN NO EVENT WILL THE COPYRIGHT HOLDER BE LIABLE TO YOU FOR DAMAGES,         
//    INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING 
//    OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED   
//    TO LOSS OF DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY    
//    YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER  
//    PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE       
//    POSSIBILITY OF SUCH DAMAGES.                                                
//                                                                                

#include "conio.h"
#include "menu.h"
#include "smpc.h"
#include "vdp.h"

#include "shared.h"

#include "SEGA_TIM.H"

void sc_tmr_start(void)
{
// From sega_tim.h :
 // * NAME:    TIM_FRT_INIT            - FRT初期化 ("init free run timer")
 // *
 // * PARAMETERS :
 // *      (1) Uint32 mode             - <i>   分周数 (parameter1: clock divider)
// #define TIM_B_CKS0      (0)                             /* CKS0              */
//  #define TIM_CKS_8       ( 0 << TIM_B_CKS0) -> 0             /* 8 カウント        */
//  #define TIM_CKS_32      ( 1 << TIM_B_CKS0) -> 1             /* 32カウント        */
//  #define TIM_CKS_128     ( 2 << TIM_B_CKS0) -> 2             /* 128カウント       */
//  #define TIM_CKS_OUT     ( 3 << TIM_B_CKS0) -> 3             /* 外部カウント      */


    TIM_FRT_INIT(TIM_CKS_32); // TIM_CKS_8, TIM_CKS_32 or TIM_CKS_128
    TIM_FRT_SET_16(0);
}

unsigned short sc_tmr_lap(void)
{
    return (TIM_FRT_GET_16());
}


// Q&D : use fixed values to convert tick to usec. (-> timing may change with HW region)
//     sc_tmr_tick2usec(60000)=71444
//     sc_tmr_tick2usec(10000)=11907
//     sc_tmr_tick2usec(2^14)=19508
// Note : In order to avoid overflows, tick can't be greater than 155000.
unsigned long sc_tmr_tick2usec(unsigned long tick)
{
#if 1
    Float32 f = TIM_FRT_CNT_TO_MCR(tick);
    return ((unsigned long)(f));
#else
    unsigned long ret = tick;
    ret = ret*19508;
    ret = ret >> 14;
    return ret;
#endif
}

void sc_tick_sleep(unsigned short tick)
{
    sc_tmr_start();
    while(TIM_FRT_GET_16() < tick);
}
void sc_usleep(unsigned long usec)
{
    /* Convert delay to tick value. */
    unsigned long delay_tick;

    if(usec <= 50*1000)
    { /* Compute tick value at good precision for delay lower than 50 msec. */

        delay_tick = (usec*60000) / (sc_tmr_tick2usec(60000));

        // Changed to delay_tick = (usec*55040) / (sc_tmr_tick2usec(55040)); where sc_tmr_tick2usec(55040) = 64K
        // delay_tick = (usec*55040);
        // delay_tick = delay_tick >> 16;
    }
    else
    { /* Poor precision, but no overflow (up to 42 seconds) for higher values. */
        delay_tick = (usec*100) / (sc_tmr_tick2usec(100));

        // Changed to delay_tick = (usec*108) / (sc_tmr_tick2usec(108)); where sc_tmr_tick2usec(108) = 128
        // delay_tick = (usec*108);
        // delay_tick = delay_tick >> 7;
    }

    /* Sleep at most 60000 ticks. */
    unsigned long i;
    for(i=0; i<delay_tick; i+=60000)
    {
        unsigned long s = ((i+60000) < delay_tick ? 60000 : delay_tick - i);
        sc_tick_sleep(s);
    }
}



// WARNING: writing to sound RAM can make Yabause and real Saturn crash
//		    I experienced this in both, this tool and the snes emu

typedef struct BENCH {
    char*	name;
#define BENCH_TITLE_LEN 32
    char	title[BENCH_TITLE_LEN];	// Buffer for use when displaying results
    unsigned long	adr;
    unsigned long	u;	// transfer unit (number of bytes per transfer)
    unsigned long	sz;	// Memory size (byte unit) to bench
    void (*function)(struct BENCH *);	// benchmark function
    char*	desc;	// benchmark description
    int		status;	// Fail/pass status
    unsigned long cycles;	// benchmark result (cycles count)
} Bench;

/* Compare access on 128KB buffer. */
#define TEST_BUFFER_SZ (0x20000)
/* Buffer 1 : contains reference data. */
#define TEST_BUFFER1		(0x2640000)
/* Buffer 2 : contains data from memory to test. */
#define TEST_BUFFER2		(0x2660000)

#define ACCESS_WRITE 0
#define ACCESS_READ  1

/* Memory test pattern definition. */
enum {PATTERN_ZERO, PATTERN_A5A5, PATTERN_INCR, PATTERN_RAND, PATTERN_CNT};
int test_pattern = PATTERN_INCR;
char *pattern_names[PATTERN_CNT] = {
    "Zero            ",
    "0xA5A5          ",
    "Increment value ",
    "Random value    ",
};

void test_pattern_fill(void)
{
    unsigned long i;
    unsigned char* test_buff1 = (unsigned char*)(TEST_BUFFER1);
    unsigned char* test_buff2 = (unsigned char*)(TEST_BUFFER2);

    for(i=0; i<TEST_BUFFER_SZ; i++)
    {
        switch(test_pattern)
        {
        default:
        case(PATTERN_ZERO):
            test_buff1[i] = 0x00;
            break;
        case(PATTERN_A5A5):
            test_buff1[i] = 0xA5;
            break;
        case(PATTERN_INCR):
            test_buff1[i] = i & 0xFF;
            break;
        case(PATTERN_RAND):
            test_buff1[i] = rand() & 0xFF;
            break;
        }
        test_buff2[i] = ~(test_buff1[i]);
    }
}

#define MAKE_FUNCTION(NAME, TYPE, ACCS_READ)			\
void NAME(Bench *bench)		\
{\
    unsigned long u = bench->u;\
    volatile register unsigned long i;\
    volatile register unsigned long n = bench->sz / u;\
    volatile register TYPE *adr = (TYPE *)bench->adr;\
\
    if(ACCS_READ)\
    {\
        sc_tmr_start();\
        for(i = n; i; i--)\
        {\
            *((adr)++);\
        }\
        bench->cycles = TIM_FRT_GET_16();\
    }\
    else\
    {\
        volatile register TYPE tmp = (TYPE)0;\
        sc_tmr_start();\
        for(i = n; i; i--)\
        {\
            *((adr)++) = tmp;\
        }\
        bench->cycles = TIM_FRT_GET_16();\
    }\
\
    test_pattern_fill();\
\
    if(ACCS_READ)\
    {\
        volatile register TYPE *buff = (TYPE *)TEST_BUFFER2;\
        adr = (TYPE *)bench->adr;\
        n = TEST_BUFFER_SZ / u;\
        memcpy((void*)adr, (void*)TEST_BUFFER1, TEST_BUFFER_SZ);\
        for(i = n; i; i--)\
        {\
            *(buff++) = *((adr)++);\
        }\
        bench->status = memcmp((void*)TEST_BUFFER2, (void*)TEST_BUFFER1, TEST_BUFFER_SZ) == 0 ? 1 : 0;\
    }\
    else\
    {\
        volatile register TYPE *buff = (TYPE *)TEST_BUFFER1;\
        adr = (TYPE *)bench->adr;\
        n = TEST_BUFFER_SZ / u;\
        for(i = n; i; i--)\
        {\
            *((adr)++) = *(buff++);\
        }\
        memcpy((void*)TEST_BUFFER2, (void*)bench->adr, TEST_BUFFER_SZ);\
        bench->status = memcmp((void*)TEST_BUFFER2, (void*)TEST_BUFFER1, TEST_BUFFER_SZ) == 0 ? 1 : 0;\
    }\
}


MAKE_FUNCTION(read8  , unsigned char , ACCESS_READ );
MAKE_FUNCTION(read16 , unsigned short, ACCESS_READ );
MAKE_FUNCTION(read32 , unsigned long , ACCESS_READ );
MAKE_FUNCTION(write8 , unsigned char , ACCESS_WRITE);
MAKE_FUNCTION(write16, unsigned short, ACCESS_WRITE);
MAKE_FUNCTION(write32, unsigned long , ACCESS_WRITE);

//* Removed tests for VDPs and sound RAM, because this tool focus on cartridge only.
#define ADR_HIGH_WORK_RAM		(0x0680000)
#define ADR_LOW_WORK_RAM		(0x0200000)
//*#define ADR_VDP1_RAM			(0x5C00000)
//*#define ADR_VDP2_RAM			(0x5E00000)
//*#define ADR_SOUND_RAM		(0x5A00000)
#define ADR_CART_RAM			(0x2400000)

#define SIZE_HIGH_WORK_RAM		(0x80000)
#define SIZE_LOW_WORK_RAM		(0x80000)//(0x100000)
//*#define SIZE_VDP1_RAM		(0x80000)
//*#define SIZE_VDP2_RAM		(0x60000)		// don't erase font
//*#define SIZE_SOUND_RAM		(0x80000)
#define SIZE_CART_RAM			(0x80000)		// only test 512 kB of DRAM0, common to 1MB and 4MB carts

Bench benches[] = {
    {"r08_hi", "", ADR_HIGH_WORK_RAM, sizeof(unsigned char ), SIZE_HIGH_WORK_RAM, read8  , "", 0, 0},	
    {"r08_lo", "", ADR_LOW_WORK_RAM , sizeof(unsigned char ), SIZE_LOW_WORK_RAM , read8  , "", 0, 0},	
//* {"r08_v1", "", ADR_VDP1_RAM     , sizeof(unsigned char ), SIZE_VDP1_RAM     , read8  , "", 0, 0},	
//* {"r08_v2", "", ADR_VDP2_RAM     , sizeof(unsigned char ), SIZE_VDP2_RAM     , read8  , "", 0, 0},	
//* {"r08_so", "", ADR_SOUND_RAM    , sizeof(unsigned char ), SIZE_SOUND_RAM    , read8  , "", 0, 0},	
    {"r08_ca", "", ADR_CART_RAM     , sizeof(unsigned char ), SIZE_CART_RAM     , read8  , "", 0, 0},	

    {"r16_hi", "", ADR_HIGH_WORK_RAM, sizeof(unsigned short), SIZE_HIGH_WORK_RAM, read16 , "", 0, 0},	
    {"r16_lo", "", ADR_LOW_WORK_RAM , sizeof(unsigned short), SIZE_LOW_WORK_RAM , read16 , "", 0, 0},	
//* {"r16_v1", "", ADR_VDP1_RAM     , sizeof(unsigned short), SIZE_VDP1_RAM     , read16 , "", 0, 0},	
//* {"r16_v2", "", ADR_VDP2_RAM     , sizeof(unsigned short), SIZE_VDP2_RAM     , read16 , "", 0, 0},	
//* {"r16_so", "", ADR_SOUND_RAM    , sizeof(unsigned short), SIZE_SOUND_RAM    , read16 , "", 0, 0},	
    {"r16_ca", "", ADR_CART_RAM     , sizeof(unsigned short), SIZE_CART_RAM     , read16 , "", 0, 0},	

    {"r32_hi", "", ADR_HIGH_WORK_RAM, sizeof(unsigned long ), SIZE_HIGH_WORK_RAM, read32 , "", 0, 0},	
    {"r32_lo", "", ADR_LOW_WORK_RAM , sizeof(unsigned long ), SIZE_LOW_WORK_RAM , read32 , "", 0, 0},	
//* {"r32_v1", "", ADR_VDP1_RAM     , sizeof(unsigned long ), SIZE_VDP1_RAM     , read32 , "", 0, 0},	
//* {"r32_v2", "", ADR_VDP2_RAM     , sizeof(unsigned long ), SIZE_VDP2_RAM     , read32 , "", 0, 0},	
//* {"r32_so", "", ADR_SOUND_RAM    , sizeof(unsigned long ), SIZE_SOUND_RAM    , read32 , "", 0, 0},	
    {"r32_ca", "", ADR_CART_RAM     , sizeof(unsigned long ), SIZE_CART_RAM     , read32 , "", 0, 0},	
//-----------
    {"w08_hi", "", ADR_HIGH_WORK_RAM, sizeof(unsigned char ), SIZE_HIGH_WORK_RAM, write8 , "", 0, 0},	
    {"w08_lo", "", ADR_LOW_WORK_RAM , sizeof(unsigned char ), SIZE_LOW_WORK_RAM , write8 , "", 0, 0},	
//* {"w08_v1", "", ADR_VDP1_RAM     , sizeof(unsigned char ), SIZE_VDP1_RAM     , write8 , "", 0, 0},	
//* {"w08_v2", "", ADR_VDP2_RAM     , sizeof(unsigned char ), SIZE_VDP2_RAM     , write8 , "", 0, 0},	
//	{"w08_so", "", ADR_SOUND_RAM    , sizeof(unsigned char ), SIZE_SOUND_RAM    , write8 , "", 0, 0},	
    {"w08_ca", "", ADR_CART_RAM     , sizeof(unsigned char ), SIZE_CART_RAM     , write8 , "", 0, 0},	

    {"w16_lo", "", ADR_LOW_WORK_RAM , sizeof(unsigned short), SIZE_LOW_WORK_RAM , write16, "", 0, 0},	
//* {"w16_v1", "", ADR_VDP1_RAM     , sizeof(unsigned short), SIZE_VDP1_RAM     , write16, "", 0, 0},	
//* {"w16_v2", "", ADR_VDP2_RAM     , sizeof(unsigned short), SIZE_VDP2_RAM     , write16, "", 0, 0},	
//	{"w16_so", "", ADR_SOUND_RAM    , sizeof(unsigned short), SIZE_SOUND_RAM    , write16, "", 0, 0},	
    {"w16_ca", "", ADR_CART_RAM     , sizeof(unsigned short), SIZE_CART_RAM     , write16, "", 0, 0},	

    {"w32_hi", "", ADR_HIGH_WORK_RAM, sizeof(unsigned long ), SIZE_HIGH_WORK_RAM, write32, "", 0, 0},	
    {"w32_lo", "", ADR_LOW_WORK_RAM , sizeof(unsigned long ), SIZE_LOW_WORK_RAM , write32, "", 0, 0},	
//* {"w32_v1", "", ADR_VDP1_RAM     , sizeof(unsigned long ), SIZE_VDP1_RAM     , write32, "", 0, 0},	
//* {"w32_v2", "", ADR_VDP2_RAM     , sizeof(unsigned long ), SIZE_VDP2_RAM     , write32, "", 0, 0},	
//	{"w32_so", "", ADR_SOUND_RAM    , sizeof(unsigned long ), SIZE_SOUND_RAM    , write32, "", 0, 0},	
    {"w32_ca", "", ADR_CART_RAM     , sizeof(unsigned long ), SIZE_CART_RAM     , write32, "", 0, 0},	
//---------- this time cache through
    {"r08*hi", "", 0x20000000 + ADR_HIGH_WORK_RAM, sizeof(unsigned char ), SIZE_HIGH_WORK_RAM, read8  , "", 0, 0},	
    {"r08*lo", "", 0x20000000 + ADR_LOW_WORK_RAM , sizeof(unsigned char ), SIZE_LOW_WORK_RAM , read8  , "", 0, 0},	
//* {"r08*v1", "", 0x20000000 + ADR_VDP1_RAM     , sizeof(unsigned char ), SIZE_VDP1_RAM     , read8  , "", 0, 0},	
//* {"r08*v2", "", 0x20000000 + ADR_VDP2_RAM     , sizeof(unsigned char ), SIZE_VDP2_RAM     , read8  , "", 0, 0},	
//* {"r08*so", "", 0x20000000 + ADR_SOUND_RAM    , sizeof(unsigned char ), SIZE_SOUND_RAM    , read8  , "", 0, 0},	
    {"r08*ca", "", 0x20000000 + ADR_CART_RAM     , sizeof(unsigned char ), SIZE_CART_RAM     , read8  , "", 0, 0},	

    {"r16*hi", "", 0x20000000 + ADR_HIGH_WORK_RAM, sizeof(unsigned short), SIZE_HIGH_WORK_RAM, read16 , "", 0, 0},	
    {"r16*lo", "", 0x20000000 + ADR_LOW_WORK_RAM , sizeof(unsigned short), SIZE_LOW_WORK_RAM , read16 , "", 0, 0},	
//* {"r16*v1", "", 0x20000000 + ADR_VDP1_RAM     , sizeof(unsigned short), SIZE_VDP1_RAM     , read16 , "", 0, 0},	
//* {"r16*v2", "", 0x20000000 + ADR_VDP2_RAM     , sizeof(unsigned short), SIZE_VDP2_RAM     , read16 , "", 0, 0},	
//* {"r16*so", "", 0x20000000 + ADR_SOUND_RAM    , sizeof(unsigned short), SIZE_SOUND_RAM    , read16 , "", 0, 0},	
    {"r16*ca", "", 0x20000000 + ADR_CART_RAM     , sizeof(unsigned short), SIZE_CART_RAM     , read16 , "", 0, 0},	

    {"r32*hi", "", 0x20000000 + ADR_HIGH_WORK_RAM, sizeof(unsigned long ), SIZE_HIGH_WORK_RAM, read32 , "", 0, 0},	
    {"r32*lo", "", 0x20000000 + ADR_LOW_WORK_RAM , sizeof(unsigned long ), SIZE_LOW_WORK_RAM , read32 , "", 0, 0},	
//* {"r32*v1", "", 0x20000000 + ADR_VDP1_RAM     , sizeof(unsigned long ), SIZE_VDP1_RAM     , read32 , "", 0, 0},	
//* {"r32*v2", "", 0x20000000 + ADR_VDP2_RAM     , sizeof(unsigned long ), SIZE_VDP2_RAM     , read32 , "", 0, 0},	
//* {"r32*so", "", 0x20000000 + ADR_SOUND_RAM    , sizeof(unsigned long ), SIZE_SOUND_RAM    , read32 , "", 0, 0},	
    {"r32*ca", "", 0x20000000 + ADR_CART_RAM     , sizeof(unsigned long ), SIZE_CART_RAM     , read32 , "", 0, 0},	
//--------
    {"w08*hi", "", 0x20000000 + ADR_HIGH_WORK_RAM, sizeof(unsigned char ), SIZE_HIGH_WORK_RAM, write8 , "", 0, 0},	
    {"w08*lo", "", 0x20000000 + ADR_LOW_WORK_RAM , sizeof(unsigned char ), SIZE_LOW_WORK_RAM , write8 , "", 0, 0},	
//* {"w08*v1", "", 0x20000000 + ADR_VDP1_RAM     , sizeof(unsigned char ), SIZE_VDP1_RAM     , write8 , "", 0, 0},	
//* {"w08*v2", "", 0x20000000 + ADR_VDP2_RAM     , sizeof(unsigned char ), SIZE_VDP2_RAM     , write8 , "", 0, 0},	
//	{"w08*so", "", 0x20000000 + ADR_SOUND_RAM    , sizeof(unsigned char ), SIZE_SOUND_RAM    , write8 , "", 0, 0},	
//* {"w08*ca", "", 0x20000000 + ADR_CART_RAM     , sizeof(unsigned char ), SIZE_CART_RAM     , write8 , "", 0, 0},	

    {"w16*hi", "", 0x20000000 + ADR_HIGH_WORK_RAM, sizeof(unsigned short), SIZE_HIGH_WORK_RAM, write16, "", 0, 0},	
    {"w16*lo", "", 0x20000000 + ADR_LOW_WORK_RAM , sizeof(unsigned short), SIZE_LOW_WORK_RAM , write16, "", 0, 0},	
//* {"w16*v1", "", 0x20000000 + ADR_VDP1_RAM     , sizeof(unsigned short), SIZE_VDP1_RAM     , write16, "", 0, 0},	
//* {"w16*v2", "", 0x20000000 + ADR_VDP2_RAM     , sizeof(unsigned short), SIZE_VDP2_RAM     , write16, "", 0, 0},	
//	{"w16*so", "", 0x20000000 + ADR_SOUND_RAM    , sizeof(unsigned short), SIZE_SOUND_RAM    , write16, "", 0, 0},	
    {"w16*ca", "", 0x20000000 + ADR_CART_RAM     , sizeof(unsigned short), SIZE_CART_RAM     , write16, "", 0, 0},	

    {"w32*hi", "", 0x20000000 + ADR_HIGH_WORK_RAM, sizeof(unsigned long ), SIZE_HIGH_WORK_RAM, write32, "", 0, 0},	
    {"w32*lo", "", 0x20000000 + ADR_LOW_WORK_RAM , sizeof(unsigned long ), SIZE_LOW_WORK_RAM , write32, "", 0, 0},	
//* {"w32*v1", "", 0x20000000 + ADR_VDP1_RAM     , sizeof(unsigned long ), SIZE_VDP1_RAM     , write32, "", 0, 0},	
//* {"w32*v2", "", 0x20000000 + ADR_VDP2_RAM     , sizeof(unsigned long ), SIZE_VDP2_RAM     , write32, "", 0, 0},	
//	{"w32*so", "", 0x20000000 + ADR_SOUND_RAM    , sizeof(unsigned long ), SIZE_SOUND_RAM    , write32, "", 0, 0},	
    {"w32*ca", "", 0x20000000 + ADR_CART_RAM     , sizeof(unsigned long ), SIZE_CART_RAM     , write32, "", 0, 0},
//--------
    {"r08/ca", "", ADR_CART_RAM                  , sizeof(unsigned char ), 512*1024          , read8  , "", 0, 0},	
    {"r16/ca", "", ADR_CART_RAM                  , sizeof(unsigned short), 512*1024          , read16 , "", 0, 0},	
    {"r32/ca", "", ADR_CART_RAM                  , sizeof(unsigned long ), 512*1024          , read32 , "", 0, 0},	
    {"r08|ca", "", 0x20000000 + ADR_CART_RAM     , sizeof(unsigned char ), 512*1024          , read8  , "", 0, 0},	
    {"r16|ca", "", 0x20000000 + ADR_CART_RAM     , sizeof(unsigned short), 512*1024          , read16 , "", 0, 0},	
    {"r32|ca", "", 0x20000000 + ADR_CART_RAM     , sizeof(unsigned long ), 512*1024          , read32 , "", 0, 0},	
};
unsigned long nBenches = sizeof(benches) / sizeof(Bench);

void bench_display_clear(void)
{
    int i;
    for(i=4; i<27; i++)
    {
        conio_printf(2, i, COLOR_WHITE, empty_line);
    }
}


/* Benchmark settings. */
unsigned char init = 8;
unsigned char hexedit_on_error = 0;

/* Stuff in order to edit benchmark settings. */
void display_bench_settings(int id)
{
    int row = 23;
    /* Display informations about settings. */
    conio_printf(2, row++, COLOR_SILVER, "Test pattern    : %s", pattern_names[test_pattern]);
    conio_printf(2, row++, COLOR_SILVER, "Hexedit on error: %s", hexedit_on_error ? "yes" : "no ");
}

/* Stuff in order to display benchmark results. */
void get_bench_name(int item_id, char** str_ptr, int* str_len_ptr)
{
    *str_ptr = (char*)(benches[item_id].title);
    sprintf(*str_ptr, "%s[%5d][%s]", benches[item_id].name, (int)(benches[item_id].cycles), benches[item_id].status ? "Pass" : "FAIL");
    *str_len_ptr = 0;
}
#define BENCH_RESULT_STARTROW 21
void display_bench_result(int id)
{
    int row = BENCH_RESULT_STARTROW;
    Bench* bench = benches+id;
    unsigned long size;
    unsigned long duration_usec;
    unsigned long cycles = bench->cycles;
    unsigned long mb_per_sec_100; // MB/sec * 100

    duration_usec = sc_tmr_tick2usec(cycles);

    /* Avoid divide by zero error. */
    if(cycles == 0) cycles = 1;

    /* Compute speed in several units. */
    size = bench->sz;

    mb_per_sec_100 = size;
    mb_per_sec_100 *= 1000;
    mb_per_sec_100 /= 1024;
    mb_per_sec_100 *= 1000;
    mb_per_sec_100 /= duration_usec;
    mb_per_sec_100 *= 100;
    mb_per_sec_100 /= 1024;

    /* Display benchmark results. */
    conio_printf(2, row++, COLOR_WHITE, "Name   : %s", bench->name);
    conio_printf(2, row++, COLOR_WHITE, "Adr:0x%08X[%d], Sz:%7u KB", (int)(bench->adr), bench->u, size>>10);
    conio_printf(2, row++, COLOR_WHITE, "Cycles : %5u (%6u.%03u msec)", bench->cycles, duration_usec/1000, duration_usec%1000);
    conio_printf(2, row++, COLOR_WHITE, "Speed  : %5u.%02u MB/sec", mb_per_sec_100/100, mb_per_sec_100%100);
}


void benchmark(void)
{
    unsigned long i;
    int row, col;

    menu_settings_t* mset   = &_menu_settings;
    menu_items_t   * mitems = &_menu_items;
    int menu_selected;

    /* Init timer stuff. */
    sc_tmr_start();

    menu_reset_settings(mset);
    menu_reset_items(mitems);
    menu_set_title(mset, "Saturn Memory Benchmark");

    menu_set_item(mitems, 0/*id*/, "Start Benchmarking    ", 0/*tag*/);
    menu_set_item(mitems, 1/*id*/, "Init DRAM             ", 1/*tag*/);
    menu_set_item(mitems, 2/*id*/, "Select test pattern   ", 2/*tag*/);
    menu_set_item(mitems, 3/*id*/, "Hexedit on error      ", 3/*tag*/);

    menu_set_pos(mset, -1/*x0*/, MENU_TOP_ROW/*y0*/, 30/*w*/, 7/*h*/, 1/*cols*/);
    menu_set_erase_on_exit(mset, 0);
    menu_set_callback_redraw(mset, display_bench_settings);

    menu_selected = 0;
    do
    {
        menu_selected = menu_start(mitems, mset, menu_selected/*selected_tag*/);
        if(mset->exit_code == MENU_EXIT_CANCEL)
        {
            return;
        }

        switch(menu_selected)
        {
        default:
        case(0):
            break;

        case(1):
            menu_clear_display(mset);
            my_RB_CartRAM_init(0/*CS0*/);
            bench_display_clear();
            break;

        case(2):
            test_pattern = (test_pattern+1)%PATTERN_CNT;
            break;

        case(3):
            hexedit_on_error = hexedit_on_error ? 0 : 1;
            break;
        }
    } while(menu_selected != 0);

    /* User requested benchmark start, so clear settings menu. */
    menu_clear_display(mset);


    /* Execute benchmarks. */
    bench_display_clear();
    row = 5, col = 2;
    for(i = 0; i < nBenches; i++)
    {
        unsigned short color;

        conio_printf(2, 4, COLOR_GREEN, "[%3d%%]Benchmarking \"%s\" ...", (100*i)/nBenches, benches[i].name);

        /* Perform bench. */
        benches[i].function(benches + i);

        /* Display some data in order to keep user waiting ... */
        display_bench_result(i);

        color = benches[i].status ? COLOR_YELLOW : COLOR_RED;
        conio_printf(col+ 0, row, color, "%s", benches[i].name);
        if(col==2)
        {
            col = 9;
        }
        else if(col==9)
        {
            col = 16;
        }
        else if(col==16)
        {
            col = 23;
        }
        else if(col==23)
        {
            col = 30;
        }
        else
        {
            col = 2;
            row++;
        }

        if(row >= BENCH_RESULT_STARTROW)
        {
            bench_display_clear();
            row = 5;
            col = 2;
        }
    }

prompt_start();//TMP?
    /* Display benchmark results. */
    bench_display_clear();

    menu_reset_settings(mset);
    menu_reset_items(mitems);
    menu_set_title(mset, "Benchmark Results");

    menu_set_pos(mset, -1/*x0*/, MENU_TOP_ROW/*y0*/, 30/*w*/, 12/*h*/, 1/*cols*/);
    menu_set_erase_on_exit(mset, 1);

    menu_set_features(mset, MENU_FEATURES_TEXTCENTER);
    menu_set_callback_getstr(mset, get_bench_name);
    menu_set_callback_redraw(mset, display_bench_result);

    menu_selected = 0;
    menu_selected = menu_list_start(NULL/*Items Array*/, nBenches, mset, menu_selected, NULL/*selection*/);
}


