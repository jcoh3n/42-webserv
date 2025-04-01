CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98
INCLUDES = -I./include
NAME = webserv

SRC_DIR = src
OBJ_DIR = obj
TEST_DIR = tests

# Sources HTTP Request
HTTP_REQUEST_SRCS = $(SRC_DIR)/http/HttpRequest.cpp \
           $(SRC_DIR)/http/utils/HttpUtils.cpp \
           $(SRC_DIR)/http/parser/FormData.cpp \
           $(SRC_DIR)/http/parser/FormParser.cpp

# Sources HTTP Response
HTTP_RESPONSE_SRCS = $(SRC_DIR)/http/HttpResponse.cpp \
           $(SRC_DIR)/http/ResponseHandler.cpp \
           $(SRC_DIR)/http/utils/FileUtils.cpp

# Sources Routes
ROUTE_SRCS = $(SRC_DIR)/http/RouteHandler.cpp

# Toutes les sources HTTP
HTTP_SRCS = $(HTTP_REQUEST_SRCS) $(HTTP_RESPONSE_SRCS) $(ROUTE_SRCS)

# Sources principales (incluant les fichiers HTTP)
SRCS = $(wildcard $(SRC_DIR)/*.cpp) \
       $(wildcard $(SRC_DIR)/*/*.cpp) \
       $(HTTP_SRCS)

# Objets principaux
OBJS = $(sort $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS)))

# Tests
TEST_PARSER = test_parser
TEST_FORM = test_form
TEST_RESPONSE = test_response
TEST_INTEGRATION = test_integration
TEST_PARSER_SRC = $(TEST_DIR)/test_parser.cpp
TEST_FORM_SRC = $(TEST_DIR)/test_form_parser.cpp
TEST_RESPONSE_SRC = $(TEST_DIR)/test_response.cpp
TEST_INTEGRATION_SRC = $(TEST_DIR)/test_integration.cpp

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
test: test_parser test_form test_response test_integration

test_parser: 
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(HTTP_SRCS) $(TEST_PARSER_SRC) -o $(TEST_PARSER)
	./$(TEST_PARSER)

test_form: 
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(HTTP_SRCS) $(TEST_FORM_SRC) -o $(TEST_FORM)
	./$(TEST_FORM)

test_response:
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(HTTP_SRCS) $(TEST_RESPONSE_SRC) -o $(TEST_RESPONSE)
	./$(TEST_RESPONSE)
	
test_integration:
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(HTTP_SRCS) $(TEST_INTEGRATION_SRC) -o $(TEST_INTEGRATION)
	./$(TEST_INTEGRATION)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)
	rm -f $(TEST_PARSER)
	rm -f $(TEST_FORM)
	rm -f $(TEST_RESPONSE)
	rm -f $(TEST_INTEGRATION)

re: fclean all

.PHONY: all clean fclean re test test_parser test_form test_response test_integration

