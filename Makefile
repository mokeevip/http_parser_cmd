GCC = gcc

TARGET = http_parser_cmd

CFLAGS := -Wall
CFLAGS += -O3
CFLAGS += -Ihttp-parser
CFLAGS += -DHTTP_PARSER_STRICT=0

SRCS = main.c http-parser/http_parser.c
$(info Source files: $(SRCS))

all: $(TARGET)
	@echo "Build Complete: $(TARGET)"

$(TARGET): $(SRCS)
	@$(GCC) $(CFLAGS) -o $@ $^

clean:
	@rm -f $(TARGET)
	@echo "Clean complete"

.PHONY: all clean