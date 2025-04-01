CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98
INCLUDES = -I./include
NAME = webserv

SRC_DIR = src
OBJ_DIR = obj
TEST_DIR = tests

# Sources HTTP
HTTP_SRCS = $(SRC_DIR)/http/HttpRequest.cpp \
           $(SRC_DIR)/http/utils/HttpUtils.cpp \
           $(SRC_DIR)/http/parser/FormData.cpp \
           $(SRC_DIR)/http/parser/FormParser.cpp

# Sources principales (incluant les fichiers HTTP)
SRCS = $(wildcard $(SRC_DIR)/*.cpp) \
       $(wildcard $(SRC_DIR)/*/*.cpp) \
       $(HTTP_SRCS)

# Objets principaux
OBJS = $(sort $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS)))

# Tests
TEST_PARSER = test_parser
TEST_FORM = test_form
TEST_PARSER_SRC = $(TEST_DIR)/test_parser.cpp
TEST_FORM_SRC = $(TEST_DIR)/test_form_parser.cpp

# Règle par défaut : compile uniquement le serveur
all: $(NAME)

# Création des répertoires d'objets
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Compilation du serveur
$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

# Tests optionnels
test: test_parser test_form

test_parser: 
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(HTTP_SRCS) $(TEST_PARSER_SRC) -o $(TEST_PARSER)
	./$(TEST_PARSER)

test_form: 
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(HTTP_SRCS) $(TEST_FORM_SRC) -o $(TEST_FORM)
	./$(TEST_FORM)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)
	rm -f $(TEST_PARSER)
	rm -f $(TEST_FORM)

re: fclean all

.PHONY: all clean fclean re test test_parser test_form

