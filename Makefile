TARGETS = revbytes

all: $(TARGETS)

.PHONY: clean all

clean:
	-rm $(TARGETS)

AUG_CFLAGS = $(CPPFLAGS) $(CFLAGS)

.c:
	$(CC) $(AUG_CFLAGS) $(LDFLAGS) -o $@ $<
