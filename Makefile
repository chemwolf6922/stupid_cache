CFLAGS?=-O3
CFLAGS+=-MMD -MP
LDFLAGS?=

LIBMAP_DIR=map-xxhash
LIBMAP_NAME=libmap.a
LIBMAP=$(LIBMAP_DIR)/$(LIBMAP_NAME)

LIB_SRC=cache.c
TEST_SRC=test.c $(LIB_SRC)

.PHONY:all
all:test lib

test:$(patsubst %.c,%.o,$(TEST_SRC)) $(LIBMAP)
	$(CC) $(LDFLAGS) -o $@ $^

.PHONY:lib
lib:libcache.a

libcache.a:$(patsubst %.c,%.o,$(LIB_SRC)) $(LIBMAP)
	$(AR) -x $(LIBMAP)
	$(AR) -rcs -o $@ *.o

$(LIBMAP):$(LIBMAP_DIR)
	$(MAKE) -C $< $(LIBMAP_NAME)

%.o:%.c
	$(CC) $(CFLAGS) -c $<

-include $(TEST_SRC:.c=.d)

clean:
	$(MAKE) -C $(LIBMAP_DIR) clean
	rm -f *.o *.d test libcache.a
