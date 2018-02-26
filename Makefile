#CFLAGS ?= -Os -Wall -std=gnu99 -I.
#LDFLAGS ?= -Os

#CFLAGS ?= -Os -Werror -Wall -Wextra -pedantic -std=gnu99 -I.
#CFLAGS_DEBUG ?= -g -Werror -Wall -Wextra -pedantic -std=gnu99 -I.

CFLAGS ?= -Os -Wall -Wextra -std=gnu99 -I.
CFLAGS_DEBUG ?= -g -Wall -Wextra -std=gnu99 -I.

SEEDBIN := seed
SEEDDEBUG := seed_debug

all: $(SEEDBIN) $(SEEDDEBUG)
	strip seed

seed.lisp.h: seed.lisp
	python3 include_file.py seed.lisp seed.lisp.h

seed: seed.lisp.h seed.c minilisp.c
	gcc $(CFLAGS) seed.c -o seed

seed_debug: seed.lisp.h seed.c
	gcc $(CFLAGS_DEBUG) seed.c -o seed_debug

clean:
	-rm seed seed.lisp.h seed_debug

.PHONY: clean
