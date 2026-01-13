BUILD_DIR := build

JOBS ?= 12

all: $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake .. && make -j$(JOBS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	if [ -d $(BUILD_DIR) ]; then rm -rf $(BUILD_DIR); fi

.PHONY: all clean
