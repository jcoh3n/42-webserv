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

# Sources Upload
UPLOAD_SRCS = $(SRC_DIR)/http/upload/FileUploadHandler.cpp \
              $(SRC_DIR)/http/upload/UploadConfig.cpp

# Sources HTTP Response
HTTP_RESPONSE_SRCS = $(SRC_DIR)/http/HttpResponse.cpp \
           $(SRC_DIR)/http/ResponseHandler.cpp \
           $(SRC_DIR)/http/utils/FileUtils.cpp \
           $(SRC_DIR)/http/utils/HttpStringUtils.cpp

# Sources Routes
ROUTE_SRCS = $(SRC_DIR)/http/RouteHandler.cpp \
             $(SRC_DIR)/http/CGIHandler.cpp

# Sources Configuration
CONFIG_SRCS = $(SRC_DIR)/config/ConfigParser.cpp \
              $(SRC_DIR)/config/ConfigUtils.cpp \
              $(SRC_DIR)/config/ConfigValidator.cpp \
              $(SRC_DIR)/config/ConfigSelector.cpp

# Toutes les sources HTTP
HTTP_SRCS = $(HTTP_REQUEST_SRCS) $(HTTP_RESPONSE_SRCS) $(ROUTE_SRCS) $(UPLOAD_SRCS)

# Sources principales
SRCS = $(wildcard $(SRC_DIR)/*.cpp) \
       $(wildcard $(SRC_DIR)/*/*.cpp) \
       $(HTTP_SRCS) \
       $(CONFIG_SRCS)

# Objets principaux
OBJS = $(sort $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS)))

# Tests
TEST_PARSER = test_parser
TEST_FORM = test_form
TEST_RESPONSE = test_response
TEST_HTTP_INTEGRATION = test_http_integration
TEST_CGI_UPLOAD = test_cgi_upload
TEST_CONFIG = test_config
TEST_CGI_SIMPLE = test_cgi_simple
TEST_UPLOAD = test_upload

TEST_PARSER_SRC = $(TEST_DIR)/unit/test_parser.cpp
TEST_FORM_SRC = $(TEST_DIR)/unit/test_form_parser.cpp
TEST_RESPONSE_SRC = $(TEST_DIR)/unit/test_response.cpp
TEST_HTTP_INTEGRATION_SRC = $(TEST_DIR)/integration/http_integration_test.cpp
TEST_CGI_UPLOAD_SRC = $(TEST_DIR)/integration/cgi_upload_test.cpp
TEST_CONFIG_SRC = $(TEST_DIR)/unit/test_config.cpp
TEST_CGI_SIMPLE_SRC = $(TEST_DIR)/test_cgi_simple.cpp
TEST_UPLOAD_SRC = $(TEST_DIR)/unit/test_upload.cpp

# Règle par défaut : compile uniquement le serveur
all: $(NAME)

# Création des répertoires d'objets
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Compilation du serveur
$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

# Tests
test: test_unit test_integration test_cgi_simple
	@echo "\nAll tests completed.\n"

test_unit: test_parser test_form test_response test_config test_upload
	@echo "\nUnit tests completed.\n"

test_integration: test_http_integration test_cgi_upload
	@echo "\nIntegration tests completed.\n"

test_cgi_simple:
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(HTTP_SRCS) $(TEST_CGI_SIMPLE_SRC) -o $(TEST_CGI_SIMPLE)
	./$(TEST_CGI_SIMPLE)

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
	
test_http_integration:
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(HTTP_SRCS) $(TEST_HTTP_INTEGRATION_SRC) -o $(TEST_HTTP_INTEGRATION)
	./$(TEST_HTTP_INTEGRATION)

test_cgi_upload:
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(HTTP_SRCS) $(TEST_CGI_UPLOAD_SRC) -o $(TEST_CGI_UPLOAD)
	./$(TEST_CGI_UPLOAD)

test_config:
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(CONFIG_SRCS) $(TEST_CONFIG_SRC) -o $(TEST_CONFIG)
	./$(TEST_CONFIG)

test_upload:
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(HTTP_SRCS) $(TEST_UPLOAD_SRC) -o $(TEST_UPLOAD)
	./$(TEST_UPLOAD)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)
	rm -f $(TEST_PARSER)
	rm -f $(TEST_FORM)
	rm -f $(TEST_RESPONSE)
	rm -f $(TEST_HTTP_INTEGRATION)
	rm -f $(TEST_CGI_UPLOAD)
	rm -f $(TEST_CONFIG)
	rm -f $(TEST_CGI_SIMPLE)
	rm -f $(TEST_UPLOAD)

re: fclean all

.PHONY: all clean fclean re test test_unit test_integration test_parser test_form test_response test_http_integration test_cgi_upload test_config test_cgi_simple test_upload

