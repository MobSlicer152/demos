include $(ROOT)/make/rules.mk

DEMO1_SRC = main.cpp
DEMO1_DEP = $(addprefix $(OBJ)/, $(DEMO1_SRC:.cpp=.d))
DEMO1_OBJS = $(addprefix $(OBJ)/, $(DEMO1_SRC:.cpp=.o))

$(OUT)/demo1: $(DEMO1_OBJS) $(OUT)/demolib.a -lX11
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

-include $(DEMO1_DEP)
