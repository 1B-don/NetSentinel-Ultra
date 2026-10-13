CC ?= cc
BUILD := builds/host
CPPFLAGS := -Iinclude -I.
CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic -g
CORE := ap_parser.c ns_analysis.c src/core/radios.c src/core/session.c src/core/settings.c
.PHONY: test clean sanitize
test: $(BUILD)/test_parser $(BUILD)/test_analysis $(BUILD)/test_core
	$(BUILD)/test_parser
	$(BUILD)/test_analysis
	$(BUILD)/test_core
$(BUILD):
	mkdir -p $@
$(BUILD)/test_parser: ap_parser.c tests/test_parser.c ap_parser.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) ap_parser.c tests/test_parser.c -o $@
$(BUILD)/test_analysis: ns_analysis.c tests/test_analysis.c ns_analysis.h ap_parser.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) ns_analysis.c tests/test_analysis.c -o $@
$(BUILD)/test_core: $(CORE) tests/test_core.c $(wildcard include/netsentinel/*.h) ap_parser.h ns_analysis.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CORE) tests/test_core.c -o $@
sanitize:
	$(MAKE) BUILD=builds/sanitize CFLAGS='$(CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer' test
clean:
	rm -rf builds/host builds/sanitize
