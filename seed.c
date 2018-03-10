#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "seed.lisp.h"
#define NO_MAIN 1
#include "minilisp.c"


static int raw_mode = 0;
static int atexit_registered = 0;
static int g_into_repl = 0;
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
    fputs(args->car->string, stdout);
    fflush(stdout);
    return Nil;
}

// (sleep <number>) ; sleep for the given number of seconds
static Obj *prim_sleep(void *root, Obj **env, Obj **list) {
    Obj *args = eval_list(root, env, list);

    if ((length(args) < 1))
        error("wrong number of parameters for prn");
    if (args->car->type != TINT)
        error("sleep only accept numbers");
    sleep(args->car->value);
    return Nil;
}

// (read-character) ; read a character from raw input
static Obj *prim_read_character(void *root, Obj **env, Obj **list) {
	unsigned char c;
	int nread;

    nread = read(STDIN_FILENO, &c, 1);
    if (nread <= 0)
		return Nil;
    return make_int(root, (int)c);
}

// (into-repl) ; return wether seed was called with -r or not
static Obj *prim_into_repl(void *root, Obj **env, Obj **list) {
    return make_int(root, (int)g_into_repl);
}

// (load-file <path>); loads the file into a list of strings
static Obj *prim_load_file(void *root, Obj **env, Obj **list) {
    char buffer[STRING_MAX_LEN + 1];
    char *p;
    Obj *args = eval_list(root, env, list);
    DEFINE1(output);
    Obj *currobj;
    Obj *currel = *output;

    if ((length(args) < 1))
        error("wrong number of parameters for load-file");
    if (args->car->type != TSTRING)
        error("load-file only accept string");
    FILE *f;
    printf("Opening %s\n", args->car->string);
    f = fopen(args->car->string, "r");
    if (f == NULL) {
        printf("Error opening %s\n", args->car->string);
        return Nil;
    }
    while(1) {
        p = fgets(buffer, STRING_MAX_LEN, f);
        if (p == NULL) {
            fclose(f);
            return *output;
        }
        currobj = make_string(root, buffer);
        if (*output == NULL) {
            (*output) = currobj;
            printf("first line %s", buffer);
            currel = *output;
        } else {
            currel->cdr = currobj;
            currel = currel->cdr;
        }
    }
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
    // Debug flags
    //debug_gc = getEnvFlag("MINILISP_DEBUG_GC");
    //always_gc = getEnvFlag("MINILISP_ALWAYS_GC");
    always_gc = 0;
    debug_gc = 0;

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
    add_primitive(root, env, "load-file", prim_load_file);
    add_primitive(root, env, "read-character", prim_read_character);
    add_primitive(root, env, "into-repl", prim_into_repl);

    if ((argc > 1) && (!strcmp(argv[1], "-r"))) {
        printf("Entering repl\n");
	    g_into_repl = 1;
    }

    if (!g_into_repl) {
        if (enable_raw_mode(STDIN_FILENO) == -1)
            return -1;
    }

    const char *buffers[] = {seed_lisp, NULL};
    int i;
    for(i = 0; ; i++) {
        /* if (g_into_repl)
            current_buffer = NULL;
        else
        */
        current_buffer = (char *)buffers[i];
        current_index = 0;
        if ((!g_into_repl) && (current_buffer == NULL)) {
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
