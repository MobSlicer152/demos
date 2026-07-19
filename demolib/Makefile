include $(ROOT)/make/rules.mk

DEMOLIB_SRC = camera.cpp \
			  color.cpp \
			  draw.cpp \
			  linux.cpp \
			  md2.cpp \
			  noise.cpp \
			  text.cpp
DEMOLIB_DEP = $(addprefix $(OBJ)/, $(DEMOLIB_SRC:.cpp=.d))
DEMOLIB_OBJS = $(addprefix $(OBJ)/, $(DEMOLIB_SRC:.cpp=.o))

$(OUT)/demolib.a: $(DEMOLIB_OBJS)

-include $(DEMOLIB_DEP)
