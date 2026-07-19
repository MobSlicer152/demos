$(OBJ)/%.o: %.cpp
	@echo CXX $(<F)
	@mkdir -p $(OBJ)
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $< -MF ${@:.o=.d}

$(OUT)/%.a:
	@echo AR $(@F)
	@mkdir -p $(OUT)
	@$(AR) rcs $@ $^
