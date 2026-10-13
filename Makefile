CC ?= cc
BUILD := builds/host
CPPFLAGS := -Iinclude -I.
CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic -g
CORE := ap_parser.c ns_analysis.c src/core/radios.c src/core/session.c src/core/settings.c src/core/persistence.c src/core/application.c
.PHONY: test clean sanitize
test: $(BUILD)/test_parser $(BUILD)/test_analysis $(BUILD)/test_core $(BUILD)/test_application $(BUILD)/test_flipper_contract
	$(BUILD)/test_parser
	$(BUILD)/test_analysis
	$(BUILD)/test_core
	$(BUILD)/test_application
	$(BUILD)/test_flipper_contract
$(BUILD):
	mkdir -p $@
$(BUILD)/test_parser: ap_parser.c tests/test_parser.c ap_parser.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) ap_parser.c tests/test_parser.c -o $@
$(BUILD)/test_analysis: ns_analysis.c tests/test_analysis.c ns_analysis.h ap_parser.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) ns_analysis.c tests/test_analysis.c -o $@
$(BUILD)/test_core: $(CORE) tests/test_core.c tests/mock_platform.h $(wildcard include/netsentinel/*.h) ap_parser.h ns_analysis.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CORE) tests/test_core.c -o $@
sanitize:
	$(MAKE) BUILD=builds/sanitize CFLAGS='$(CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer' test
clean:
	rm -rf builds/host builds/sanitize

$(BUILD)/test_application: $(CORE) tests/test_application.c tests/mock_platform.h $(wildcard include/netsentinel/*.h) ap_parser.h ns_analysis.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CORE) tests/test_application.c -o $@

FLIPPER := src/flipper/ns_flipper.c src/flipper/netsentinel_app.c
$(BUILD)/test_flipper_contract: $(CORE) $(FLIPPER) src/flipper/ns_flipper.h tests/test_flipper_contract.c tests/flipper_mock/mock_sdk.c ap_parser.h ns_analysis.h $(wildcard tests/flipper_mock/*.h tests/flipper_mock/*/*.h) $(wildcard include/netsentinel/*.h) | $(BUILD)
	$(CC) $(CPPFLAGS) -Itests/flipper_mock -Isrc/flipper $(CFLAGS) $(CORE) $(FLIPPER) tests/flipper_mock/mock_sdk.c tests/test_flipper_contract.c -o $@
