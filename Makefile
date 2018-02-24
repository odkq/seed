#CFLAGS ?= -Os -Wall -std=gnu99 -I.
#LDFLAGS ?= -Os

CFLAGS ?= -Os -Wall -std=gnu99 -I.
LDFLAGS ?= -Os

SEEDBIN := seed

all: $(SEEDBIN)
	strip seed

seed.lisp.h: seed.lisp
	python3 include_file.py seed.lisp seed.lisp.h

seed: seed.lisp.h seed.c
	gcc $(CFLAGS) seed.c -o seed

clean:
	-rm seed seed.lisp.h

.PHONY: clean
