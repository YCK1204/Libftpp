NAME		= libftpp.a

CXX			= c++
CXXFLAGS	= -Wall -Wextra -Werror -std=c++11
CPPFLAGS	= -I. -Imandatory/header -Ibonus/header -MMD -MP
AR			= ar rcs

OBJ_DIR		= obj

# ---------------------------------------------------------------- sources

MAN_DIR		= mandatory/src
BONUS_DIR	= bonus/src

MAN_SRCS	= $(wildcard $(MAN_DIR)/*.cpp)
BONUS_SRCS	= $(wildcard $(BONUS_DIR)/*.cpp)

MAN_OBJS	= $(MAN_SRCS:%.cpp=$(OBJ_DIR)/%.o)
BONUS_OBJS	= $(BONUS_SRCS:%.cpp=$(OBJ_DIR)/%.o)

BONUS_STAMP	= $(OBJ_DIR)/.bonus

# ---------------------------------------------------------------- tests

TEST_MAN		= test_libftpp
TEST_BONUS		= test_libftpp_bonus

TEST_MAN_SRCS	= $(wildcard tests/mandatory/*.cpp)
TEST_BONUS_SRCS	= $(wildcard tests/bonus/*.cpp)

TEST_MAN_OBJS	= $(TEST_MAN_SRCS:%.cpp=$(OBJ_DIR)/%.o)
TEST_BONUS_OBJS	= $(TEST_BONUS_SRCS:%.cpp=$(OBJ_DIR)/%.o)

DEPS		= $(MAN_OBJS:.o=.d) $(BONUS_OBJS:.o=.d) \
			  $(TEST_MAN_OBJS:.o=.d) $(TEST_BONUS_OBJS:.o=.d)

# ---------------------------------------------------------------- rules

all: $(NAME)

$(NAME): $(MAN_OBJS)
	$(AR) $@ $^

bonus: $(BONUS_STAMP)

$(BONUS_STAMP): $(MAN_OBJS) $(BONUS_OBJS)
	$(AR) $(NAME) $^
	@touch $@

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

test: $(TEST_MAN)
	./$(TEST_MAN)

test_bonus: $(TEST_BONUS)
	./$(TEST_BONUS)

$(TEST_MAN): $(TEST_MAN_OBJS) $(NAME)
	$(CXX) $(CXXFLAGS) $(TEST_MAN_OBJS) $(NAME) -pthread -o $@

$(TEST_BONUS): $(TEST_BONUS_OBJS) $(BONUS_STAMP)
	$(CXX) $(CXXFLAGS) $(TEST_BONUS_OBJS) $(NAME) -pthread -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME) $(TEST_MAN) $(TEST_BONUS)

re: fclean all

-include $(DEPS)

.PHONY: all bonus clean fclean re test test_bonus
