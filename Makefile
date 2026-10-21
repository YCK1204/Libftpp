NAME		= libftpp.a

CXX			= c++
CXXFLAGS	= -Wall -Wextra -Werror -std=c++11
CPPFLAGS	= -I. -Iheader -MMD -MP
AR			= ar rcs

SRC_DIR		= src
OBJ_DIR		= obj

TEST_DIR	= tests
TEST_NAME	= test_libftpp

SRCS		= $(notdir $(wildcard $(SRC_DIR)/*.cpp))
TEST_SRCS	= $(notdir $(wildcard $(TEST_DIR)/*.cpp))

OBJS		= $(addprefix $(OBJ_DIR)/, $(SRCS:.cpp=.o))
TEST_OBJS	= $(addprefix $(OBJ_DIR)/$(TEST_DIR)/, $(TEST_SRCS:.cpp=.o))
DEPS		= $(OBJS:.o=.d) $(TEST_OBJS:.o=.d)

all: $(NAME)

$(NAME): $(OBJS)
	$(AR) $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

$(OBJ_DIR) $(OBJ_DIR)/$(TEST_DIR):
	mkdir -p $@

test: $(TEST_NAME)
	./$(TEST_NAME)

$(TEST_NAME): $(TEST_OBJS) $(NAME)
	$(CXX) $(CXXFLAGS) $(TEST_OBJS) $(NAME) -pthread -o $@

$(OBJ_DIR)/$(TEST_DIR)/%.o: $(TEST_DIR)/%.cpp | $(OBJ_DIR)/$(TEST_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TEST_OBJS) $(DEPS)

fclean: clean
	rm -f $(NAME) $(TEST_NAME)

re: fclean all

-include $(DEPS)

.PHONY: all clean fclean re test
