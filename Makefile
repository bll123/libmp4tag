#

PKGNM=libmp4tag
CLI_NM=mp4tagcli

LIB_OBJ=$(PKGNM).o parsemp4.o tagdef.o
CLI_OBJ=$(CLI_NM).o
EXE_EXT=

CFLAGS=-Wall -Wextra

.PHONY: all clean
all:
	@API=$$(grep '^#define LIBMP4TAG_API_VERS [1-9]' $(PKGNM).h \
		| sed 's,.* ,,'); \
	$(MAKE) APIVERS=$${API} $(PKGNM).so $(CLI_NM)

clean:
	-rm -f $(LIB_OBJ) $(CLI_OBJ) \
		$(PKGNM).so $(PKGNM).so.1 $(CLI_NM)$(EXE_EXT) \
		w ww *~

# libraries

$(PKGNM).so.$(APIVERS): $(LIB_OBJ)
	$(CC) -shared \
		-Wl,-soname,$(PKGNM).so.$(APIVERS) \
		-o $@ $(LIB_OBJ)

$(PKGNM).so: $(PKGNM).so.$(APIVERS)
	ln -sf $(PKGNM).so.$(APIVERS) $(PKGNM).so

# executables

$(CLI_NM)$(EXE_EXT): $(CLI_OBJ) $(PKGNM).so
	$(CC) -o $@ \
		-Wl,-R\$${ORIGIN} \
		$(CLI_OBJ) $(PKGNM).so

# object files

%.o: %.c
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

$(LIB_OBJ): $(PKGNM).h
$(CLI_OBJ): $(PKGNM).h

