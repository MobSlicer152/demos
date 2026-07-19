include $(ROOT)/make/rules.mk

DEMO2_SRC = main.cpp
DEMO2_DEP = $(addprefix $(OBJ)/, $(DEMO2_SRC:.cpp=.d))
DEMO2_OBJS = $(addprefix $(OBJ)/, $(DEMO2_SRC:.cpp=.o))

$(OUT)/demo2: $(DEMO2_OBJS) $(OUT)/demolib.a -lX11 -lXrender
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

-include $(DEMO2_DEP)
