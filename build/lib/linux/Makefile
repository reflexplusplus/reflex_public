REFLEX_ROOT := ../../..
REFLEX_SRC := $(REFLEX_ROOT)/src/reflex
REFLEX_INC := $(REFLEX_ROOT)/include
REFLEX_BIN := $(REFLEX_ROOT)/bin/lib/linux
BUILD_DIR := $(CURDIR)/out

CURL_CFLAGS ?= $(shell pkg-config --cflags libcurl 2>/dev/null)
ifeq ($(strip $(CURL_CFLAGS)),)
CURL_CFLAGS := $(shell curl-config --cflags 2>/dev/null)
endif

WAYLAND_CFLAGS ?= $(shell pkg-config --cflags wayland-client wayland-egl egl glesv2 2>/dev/null)

COMMON_CXXFLAGS := -std=c++20 -fno-rtti -pthread -I$(REFLEX_INC) -I$(REFLEX_SRC)/glx/ext/freetype-2.9.1/include $(CURL_CFLAGS) $(WAYLAND_CFLAGS)
DEBUG_CXXFLAGS := $(COMMON_CXXFLAGS) -O0 -g -DDEBUG=1
RELEASE_CXXFLAGS := $(COMMON_CXXFLAGS) -O2 -DNDEBUG

DEBUG_COMMON_OBJECTS := $(BUILD_DIR)/Debug/core.o $(BUILD_DIR)/Debug/data.o $(BUILD_DIR)/Debug/file.o $(BUILD_DIR)/Debug/vec.o
RELEASE_COMMON_OBJECTS := $(BUILD_DIR)/Release/core.o $(BUILD_DIR)/Release/data.o $(BUILD_DIR)/Release/file.o $(BUILD_DIR)/Release/vec.o

.PHONY: all debug release clean

all: debug release

debug: $(REFLEX_BIN)/Debug/libReflexCommon.a $(REFLEX_BIN)/Debug/libReflexTargetConsole.a $(REFLEX_BIN)/Debug/libReflexTargetApp.a

release: $(REFLEX_BIN)/Release/libReflexCommon.a $(REFLEX_BIN)/Release/libReflexTargetConsole.a $(REFLEX_BIN)/Release/libReflexTargetApp.a

clean:
	rm -rf $(BUILD_DIR)

$(BUILD_DIR)/Debug:
	mkdir -p $(BUILD_DIR)/Debug $(REFLEX_BIN)/Debug

$(BUILD_DIR)/Release:
	mkdir -p $(BUILD_DIR)/Release $(REFLEX_BIN)/Release

$(BUILD_DIR)/Debug/core.o: $(REFLEX_SRC)/core.cpp | $(BUILD_DIR)/Debug
	g++ $(DEBUG_CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/Debug/data.o: $(REFLEX_SRC)/data.cpp | $(BUILD_DIR)/Debug
	g++ $(DEBUG_CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/Debug/file.o: $(REFLEX_SRC)/file.cpp | $(BUILD_DIR)/Debug
	g++ $(DEBUG_CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/Debug/vec.o: $(REFLEX_SRC)/vec.cpp | $(BUILD_DIR)/Debug
	g++ $(DEBUG_CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/Release/core.o: $(REFLEX_SRC)/core.cpp | $(BUILD_DIR)/Release
	g++ $(RELEASE_CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/Release/data.o: $(REFLEX_SRC)/data.cpp | $(BUILD_DIR)/Release
	g++ $(RELEASE_CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/Release/file.o: $(REFLEX_SRC)/file.cpp | $(BUILD_DIR)/Release
	g++ $(RELEASE_CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/Release/vec.o: $(REFLEX_SRC)/vec.cpp | $(BUILD_DIR)/Release
	g++ $(RELEASE_CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/Debug/linux_console.o: $(REFLEX_SRC)/system/linux_console.cpp | $(BUILD_DIR)/Debug
	g++ $(DEBUG_CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/Debug/linux_app.o: $(REFLEX_SRC)/system/linux_app.cpp | $(BUILD_DIR)/Debug
	g++ $(DEBUG_CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/Release/linux_console.o: $(REFLEX_SRC)/system/linux_console.cpp | $(BUILD_DIR)/Release
	g++ $(RELEASE_CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/Release/linux_app.o: $(REFLEX_SRC)/system/linux_app.cpp | $(BUILD_DIR)/Release
	g++ $(RELEASE_CXXFLAGS) -c $< -o $@

$(REFLEX_BIN)/Debug/libReflexCommon.a: $(DEBUG_COMMON_OBJECTS)
	ar rcs $@ $^

$(REFLEX_BIN)/Release/libReflexCommon.a: $(RELEASE_COMMON_OBJECTS)
	ar rcs $@ $^

$(REFLEX_BIN)/Debug/libReflexTargetConsole.a: $(BUILD_DIR)/Debug/linux_console.o
	ar rcs $@ $^

$(REFLEX_BIN)/Debug/libReflexTargetApp.a: $(BUILD_DIR)/Debug/linux_app.o
	ar rcs $@ $^

$(REFLEX_BIN)/Release/libReflexTargetConsole.a: $(BUILD_DIR)/Release/linux_console.o
	ar rcs $@ $^

$(REFLEX_BIN)/Release/libReflexTargetApp.a: $(BUILD_DIR)/Release/linux_app.o
	ar rcs $@ $^
