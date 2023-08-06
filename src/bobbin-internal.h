#ifndef BOBBIN_INTERNAL_H
#define BOBBIN_INTERNAL_H

#define _XOPEN_SOURCE   700

#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <strings.h>

#include "ac-config.h"
#include "apple2.h"

typedef uint16_t    word;
typedef uint8_t     byte;

#define STREQ(a, b) (!strcmp(a, b))
#define STREQCASE(a, b) (!strcasecmp(a, b))

#define BYTE(b)         ((b) & 0xFF)
#define WORD(lo, hi)    (0xFFFF & ((BYTE(hi) << 8) | BYTE(lo)))
#define HI(w)           ((0xFF00 & (w)) >> 8)
#define LO(w)           (0x00FF & (w))

extern void bobbin_run(void);
extern word current_pc(void);

/********** LOGGING **********/

#define DIE_LEVEL      0
#define WARN_LEVEL     1
#define INFO_LEVEL     2
#define VERBOSE_LEVEL  3

#define DEFAULT_LEVEL   WARN_LEVEL

#define SQUAWK_OK(level)    ((level) <= cfg.squawk_level)
#define WARN_OK         SQUAWK_OK(WARN_LEVEL)
#define INFO_OK         SQUAWK_OK(INFO_LEVEL)
#define VERBOSE_OK      SQUAWK_OK(VERBOSE_LEVEL)

#define SQUAWK_CONT(level, ...) \
    do { \
        if (SQUAWK_OK(level)) { \
            squawk(level, 1, __VA_ARGS__); \
        } \
    } while (0)
#define SQUAWK(level, ...) \
    do { \
        if (SQUAWK_OK(level)) { \
            squawk(level, 0, __VA_ARGS__); \
        } \
    } while (0)

#define WARN(...)           SQUAWK(WARN_LEVEL, __VA_ARGS__)
#define WARN_CONT(...)      SQUAWK_CONT(WARN_LEVEL, __VA_ARGS__)
#define INFO(...)           SQUAWK(INFO_LEVEL, __VA_ARGS__)
#define INFO_CONT(...)      SQUAWK_CONT(INFO_LEVEL, __VA_ARGS__)
#define VERBOSE(...)        SQUAWK(VERBOSE_LEVEL, __VA_ARGS__)
#define VERBOSE_CONT(...)   SQUAWK_CONT(VERBOSE_LEVEL, __VA_ARGS__)

#define DIE_FINAL(st) do { \
        SQUAWK(DIE_LEVEL, "Exiting (%d).\n", (int)st); \
        exit(st); \
    } while (0)
#define DIE_CONT(st, ...) do { \
        SQUAWK_CONT(DIE_LEVEL, __VA_ARGS__); \
        if (st) DIE_FINAL(st); \
    } while(0)
#define DIE(st, ...) do { \
        SQUAWK(DIE_LEVEL, __VA_ARGS__); \
        if (st) DIE_FINAL(st); \
    } while(0)

/********** CONFIG **********/

extern const char *program_name; // main.c

typedef struct Config Config;
struct Config {
    int squawk_level;
    const char *    inputfile;
    const char *    outputfile;
    const char *    interface;

    // machine/emulation config optioons
    const char *    machine;
    bool            machine_set;
    size_t          amt_ram;
    bool            load_rom;
    const char *    ram_load_file;
    unsigned long   ram_load_loc;
    const char *    rom_load_file;
    bool            basic_fixup;
    bool            turbo;
    bool            turbo_was_set;
    bool            lang_card;
    bool            lang_card_set;
    word            start_loc;
    bool            start_loc_set;
    word            delay_until;
    bool            delay_set;
    bool            bell;

    // "simple" interface config:
    bool            remain_after_pipe;
    bool            remain_tty;
    const char *    simple_input_mode;

    // trace stuff
    bool            die_on_brk;
    const char *    trace_file;
    uintmax_t       trace_start;
    uintmax_t       trace_end;
    bool            trap_success_on;
    word            trap_success;
    bool            trap_failure_on;
    word            trap_failure;
    bool            trap_print_on;
    word            trap_print;

    // special options
    bool            watch;
    bool            tokenize;
    bool            detokenize;
};
extern Config cfg;

/********** CPU **********/

typedef struct Registers Registers;
struct Registers {
    word    pc;
    byte    sp;
    byte    p; /* status flags */
    byte    a; /* accumulator */
    byte    x;
    byte    y;
};

typedef struct Cpu Cpu;
struct Cpu {
    Registers regs;
};

extern Cpu theCpu;

#define PC      (theCpu.regs.pc)
#define SP      (theCpu.regs.sp)
#define STACK   WORD(SP, 0x01)
#define PFLAGS  (theCpu.regs.p)
#define ACC     (theCpu.regs.a)
#define XREG    (theCpu.regs.x)
#define YREG    (theCpu.regs.y)

#define PC_ADV  ++PC

#define PCARRY  0
#define PZERO   1
#define PINT    2
#define PDEC    3
#define PBRK    4
#define PUNUSED 5
#define POVERFL 6
#define PNEG    7

#define PMASK(flag)     (1 << flag)
#define PGET(flag)      (!!(PFLAGS & PMASK(flag)))
#define PPUT(flag, val) ((void)(PFLAGS = ((PFLAGS & ~PMASK(flag)) \
                                   | (!!(val) << flag))))
#define PTEST(flag)     (!!(PFLAGS & PMASK(flag)))

#define RPGET(p, flag)      (!!((p) & PMASK(flag)))
#define RPPUT(p, flag, val) ((void)((p) = (((p) & ~PMASK(flag)) \
                                   | (!!(val) << flag))))
#define RPTEST(p, flag)     (!!((p) & PMASK(flag)))

extern void cpu_reset(void);
extern void cpu_step(void);

static inline void go_to(word w) {
    PC = w;
}

/********** MACHINE **********/

size_t expected_rom_size(void);
extern const char *default_romfname;
extern bool validate_rom(unsigned char *buf, size_t sz);
extern bool machine_is_iie(void);

/********** MEMORY **********/

typedef struct RestartSw RestartSw;
struct RestartSw {
    bool ramrd : 1; // s/b auxrd
    bool ramwrt: 1; // s/b auxwrt
    bool intcxrom : 1;
    bool altzp : 1;
    bool intc8rom : 1;
    bool slotc3rom : 1;
    bool eightystore : 1;
    bool vertblank : 1;
    bool text : 1;
    bool mixed : 1;
    bool page2 : 1;
    bool hires : 1;
    bool altcharset : 1;
    bool eightycol : 1;
};
extern RestartSw rstsw;

extern void mem_init(void);
extern void mem_reset(void);
extern void mem_reboot(void);
extern const byte *getram(void);
extern byte peek(word loc);
extern void poke(word loc, byte val);
// These versions don't trigger debugger break-on-memory,
//  and don't affect or use floating bus values.
extern byte peek_sneaky(word loc);
extern void poke_sneaky(word loc, byte val);
extern bool mem_match(word loc, unsigned int nargs, ...);
extern void load_ram_finish(void);
extern size_t mem_transform_aux(word loc, bool wr);

static inline byte stack_get(void)
{
    return peek(STACK);
}

static inline byte stack_get_sneaky(void)
{
    return peek(STACK);
}

static inline void stack_put(byte val)
{
    poke(STACK, val);
}

static inline void stack_put_sneaky(byte val)
{
    poke_sneaky(STACK, val);
}

static inline byte stack_inc(void)
{
    return ++SP;
}

static inline byte stack_dec(void)
{
    return --SP;
}

static inline void stack_push(byte val)
{
    stack_put(val);
    (void) stack_dec();
}

static inline void stack_push_sneaky(byte val)
{
    stack_put_sneaky(val);
    (void) stack_dec();
}

static inline void stack_push_flags_or(byte val)
{
    stack_push(PFLAGS | (1 << PUNUSED) | val);
}

static inline byte stack_pop(void)
{
    (void) stack_inc();
    return stack_get();
}

static inline byte stack_pop_sneaky(void)
{
    (void) stack_inc();
    return stack_get_sneaky();
}

static inline byte pc_get_adv(void)
{
    byte op = peek(PC);
    PC_ADV;
    return op;
}

static inline word word_at(word loc)
{
    return WORD(peek_sneaky(loc), peek_sneaky(loc+1));
}

// s/b in CPU, but need to be decl'd after stack_pop etc
static inline void rts(void) {
    byte lo = stack_pop();
    byte hi = stack_pop();
    go_to(WORD(lo, hi)+1);
}

/********** INTERFACES **********/

typedef struct IfaceDesc IfaceDesc;
struct IfaceDesc {
    void (*init)(void); // Process options
    void (*start)(void);// *Actually* start interface
    void (*prestep)(void);
    void (*step)(void);
    int  (*peek)(word loc);
    bool (*poke)(word loc, byte val);
    void (*frame)(bool flash);
    void (*unhook)(void);
    void (*rehook)(void);
    void (*switch_chg)(void);
    bool (*squawk)(int level, bool cont, const char *fmt, va_list args);
        // returns true to suppress default squawk handling
    void (*display_touched)(void); // Display memory may have updated
                                   // behind the scenes: redraw!
};

extern void interfaces_init(void);
extern void interfaces_start(void);

extern void iface_prestep(void);
extern void iface_step(void);
extern bool iface_poke(word loc, byte val);
                                  // if -1 is NOT returned, suppress
                                  // actual memory write (because iface
                                  // intercepted the write)

extern int  iface_peek(word loc); // returns -1 if no change over "real" mem
extern void iface_frame(bool flash);
extern void iface_unhook(void);
extern void iface_rehook(void);
extern void iface_switch(void);
extern void iface_display_touched(void);
extern void squawk(int level, bool cont, const char *format, ...);

/********** HOOK **********/

extern void rh_prestep(void); // The only hooks allowed to examine and
                              // change the PC, but NOT allowed to do
                              // anything else.
extern void rh_step(void);
//extern int  rh_peek_sneaky(word loc);
extern bool rh_poke(word loc, byte val);
extern int  rh_peek(word loc);
extern void rh_reboot(void);
extern void rh_display_touched(void);
extern void rh_switch(void);

/********** TRACE **********/

extern const char *trfile_name;
extern FILE *trfile;

extern void trace_step(void);
extern void trace_on(char *format, ...);
extern void trace_off(void);
extern int  tracing(void);

/********** DEBUG **********/

typedef int (*printer)(const char * fmt, ...);
bool command_do(const char *line, printer pr);

/********** DEBUG **********/

extern void dbg_on(void);
extern void debugger(void);
extern bool debugging(void);

/********** UTIL **********/

extern void util_print_state(FILE *f, word pc, Registers *reg);
extern bool util_isflashing(int c);
extern bool util_isreversed(int c, bool flash);
extern int util_todisplay(int c);
extern int util_toascii(int c);
extern int util_fromascii(int c);
extern int util_isprint(int c);
extern void util_reopen_stdin_tty(int flags);

/********** WATCH **********/

extern void setup_watches(void);
extern bool check_watches(void);

/* TBD */
extern word print_disasm(FILE *f, word pos, Registers *regs);

#define NS_PER_FRAME        16651559
#define CYCLES_PER_FRAME    17030

extern uintmax_t cycle_count;
extern uintmax_t instr_count;
extern uintmax_t frame_count;
static inline void cycle(void) { ++cycle_count; }
extern volatile sig_atomic_t sigint_received;
extern void unhandle_sigint(void);

#endif /* BOBBIN_INTERNAL_H */
