export ROOT = $(shell pwd)
export OBJ = obj
export OUT = $(ROOT)/out

export AR := llvm-ar
export CXX := clang++
export INCLUDES += -I$(ROOT)/demolib
export CXXFLAGS += -g -std=gnu++26 -fms-extensions $(INCLUDES)
export CPPFLAGS += -MMD
export LDFLAGS += -fuse-ld=lld

.PHONY: all clean demolib demo1 demo2

all: demolib demo1 demo2

clean:
	@$(RM) -r $(OUT)
	@$(RM) -r **/$(OBJ)

demolib:
	$(MAKE) -C $(ROOT)/$@

demo1:
	$(MAKE) -C $(ROOT)/$@

demo2:
	$(MAKE) -C $(ROOT)/$@
