#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "seed.lisp.h"
#include "minilisp.c"


static int raw_mode = 0;
static int atexit_registered = 0;
static struct termios orig_termios;

static void seed_atexit(void);

// (scren-size)
// Return a list with (columns lines)
static Obj *prim_screen_size(void *root, Obj **env, Obj **list)
{
    struct winsize ws;
    DEFINE3(width, height, head);
    if (ioctl(1, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        return Nil;
    }
    *width = make_int(root, ws.ws_col);
    *height = make_int(root, ws.ws_row);
    *head = Nil;
    *head = cons(root, height, head);
    return cons(root, width, head);
}

// (prn <string>) ; print raw string
static Obj *prim_prn(void *root, Obj **env, Obj **list) {
    Obj *args = eval_list(root, env, list);

    if ((length(args) < 1))
        error("wrong number of parameters for prn");
    if (args->car->type != TSTRING)
        error("prn only accept strings");
    puts(args->car->string);
    return Nil;
}

// (sleep <number>) ; sleep for the given number of seconds
static Obj *prim_sleep(void *root, Obj **env, Obj **list) {
    Obj *args = eval_list(root, env, list);

    if ((length(args) < 1))
        error("wrong number of parameters for prn");
    if (args->car->type != TINT)
        error("sleep only accept strings");
    sleep(args->car->value);
    return Nil;
}
/* Raw mode: Taken from linenoise (see LICENSE) */
static int enable_raw_mode(int fd) {
    struct termios raw;

    if (!isatty(STDIN_FILENO)) goto fatal;
    if (!atexit_registered) {
        atexit(seed_atexit);
        atexit_registered = 1;
    }
    if (tcgetattr(fd,&orig_termios) == -1) goto fatal;

    raw = orig_termios;  /* modify the original mode */
    /* input modes: no break, no CR to NL, no parity check, no strip char,
     * no start/stop output control. */
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    /* output modes - disable post processing */
    raw.c_oflag &= ~(OPOST);
    /* control modes - set 8 bit chars */
    raw.c_cflag |= (CS8);
    /* local modes - choing off, canonical off, no extended functions,
     * no signal chars (^Z,^C) */
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    /* control chars - set return condition: min number of bytes and timer.
     * We want read to return every single byte, without timeout. */
    raw.c_cc[VMIN] = 1; raw.c_cc[VTIME] = 0; /* 1 byte, no timer */

    /* put terminal in raw mode after flushing */
    if (tcsetattr(fd,TCSAFLUSH,&raw) < 0) goto fatal;
    raw_mode = 1;
    return 0;

fatal:
    errno = ENOTTY;
    return -1;
}

static void disable_raw_mode(int fd) {
    /* Don't even check the return value as it's too late. */
    if (raw_mode && tcsetattr(fd,TCSAFLUSH,&orig_termios) != -1)
        raw_mode = 0;
}

/* At exit we'll try to fix the terminal to the initial conditions. */
static void seed_atexit(void) {
    disable_raw_mode(STDIN_FILENO);
}

int main(int argc, char **argv) {
    int repl = 0;

    // Debug flags
    debug_gc = getEnvFlag("MINILISP_DEBUG_GC");
    always_gc = getEnvFlag("MINILISP_ALWAYS_GC");

    // Memory allocation
    memory = alloc_semispace();

    // Constants and primitives
    Symbols = Nil;
    void *root = NULL;
    DEFINE2(env, expr);
    *env = make_env(root, &Nil, &Nil);
    define_constants(root, env);
    define_primitives(root, env);

    add_primitive(root, env, "screen-size", prim_screen_size);
    add_primitive(root, env, "prn", prim_prn);
    add_primitive(root, env, "sleep", prim_sleep);

    if ((argc > 1) && (!strcmp(argv[1], "-r"))) {
        printf("Entering repl\n");
	    repl = 1;
    }

    if (!repl) {
        if (enable_raw_mode(STDIN_FILENO) == -1)
            return -1;
    }

    const char *buffers[] = {seed_lisp, NULL};
    int i;
    for(i = 0; ; i++) {
        if (repl)
            current_buffer = NULL;
        else
            current_buffer = (char *)buffers[i];
        current_index = 0;
        if ((!repl) && (current_buffer == NULL)) {
            break;
        }
        // The main loop
        for (;;) {
            *expr = read_expr(root);
            if (!*expr)
                break;
            if (*expr == Cparen)
                error("Stray close parenthesis");
            if (*expr == Dot)
                error("Stray dot");
            if (current_buffer == NULL)
                print(eval(root, env, expr));
            else
                eval(root, env, expr);
        }
        if (current_buffer == NULL) {
            break;
        }
    }
}