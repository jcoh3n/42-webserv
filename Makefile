CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -I./include
NAME = webserv
TEST_PARSER = test_parser
TEST_FORM = test_form

SRC_DIR = src
OBJ_DIR = obj
TEST_DIR = tests

# Liste des fichiers source
HTTP_SRCS = $(SRC_DIR)/http/HttpRequest.cpp \
            $(SRC_DIR)/http/utils/HttpUtils.cpp \
            $(SRC_DIR)/http/parser/FormData.cpp \
            $(SRC_DIR)/http/parser/FormParser.cpp

SRCS = $(HTTP_SRCS)
TEST_PARSER_SRC = $(TEST_DIR)/test_parser.cpp
TEST_FORM_SRC = $(TEST_DIR)/test_form_parser.cpp

OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

test: test_parser test_form

test_parser: $(SRCS) $(TEST_PARSER_SRC)
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(SRCS) $(TEST_PARSER_SRC) -o $(TEST_PARSER)
	./$(TEST_PARSER)

test_form: $(SRCS) $(TEST_FORM_SRC)
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(SRCS) $(TEST_FORM_SRC) -o $(TEST_FORM)
	./$(TEST_FORM)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME) $(TEST_PARSER) $(TEST_FORM)

re: fclean all

.PHONY: all clean fclean re test test_parser test_form 