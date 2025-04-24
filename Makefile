# **************************************************************************** #
#                                    COLORS                                    #
# **************************************************************************** #
RESET       = \033[0m
BLACK       = \033[30m
RED         = \033[31m
GREEN       = \033[32m
YELLOW      = \033[33m
BLUE        = \033[34m
MAGENTA     = \033[35m
CYAN        = \033[36m
WHITE       = \033[37m
BOLD        = \033[1m
DIM         = \033[2m
UNDERLINE   = \033[4m

# Custom colors for HTTP methods (matching the site's colors)
COLOR_GET    = \033[38;2;0;186;188m
COLOR_POST   = \033[38;2;147;112;219m
COLOR_DELETE = \033[38;2;255;71;87m
COLOR_BUILD  = \033[38;2;33;150;243m
COLOR_CLEAN  = \033[38;2;255;107;129m
COLOR_TEST   = \033[38;2;46;204;113m

# **************************************************************************** #
#                                  VARIABLES                                   #
# **************************************************************************** #
CXX         = c++
CXXFLAGS    = -Wall -Wextra -Werror -std=c++98
INCLUDES    = -I./include
NAME        = webserv

# Directories
SRC_DIR     = src
OBJ_DIR     = obj
TEST_DIR    = tests

# Source files organization
HTTP_REQUEST_SRCS = $(SRC_DIR)/http/HttpRequest.cpp \
                   $(SRC_DIR)/http/utils/HttpUtils.cpp \
                   $(SRC_DIR)/http/parser/FormData.cpp \
                   $(SRC_DIR)/http/parser/FormParser.cpp

UPLOAD_SRCS       = $(SRC_DIR)/http/upload/FileUploadHandler.cpp \
                   $(SRC_DIR)/http/upload/UploadConfig.cpp

HTTP_RESPONSE_SRCS = $(SRC_DIR)/http/HttpResponse.cpp \
                    $(SRC_DIR)/http/ResponseHandler.cpp \
                    $(SRC_DIR)/http/utils/FileUtils.cpp \
                    $(SRC_DIR)/http/utils/HttpStringUtils.cpp

ROUTE_SRCS        = $(SRC_DIR)/http/RouteHandler.cpp \
                   $(SRC_DIR)/http/CGIHandler.cpp

CONFIG_SRCS       = $(SRC_DIR)/config/ConfigParser.cpp \
                   $(SRC_DIR)/config/ConfigUtils.cpp \
                   $(SRC_DIR)/config/ConfigValidator.cpp \
                   $(SRC_DIR)/config/ConfigSelector.cpp

# Group all HTTP sources
HTTP_SRCS         = $(HTTP_REQUEST_SRCS) $(HTTP_RESPONSE_SRCS) $(ROUTE_SRCS) $(UPLOAD_SRCS)

# All sources
SRCS              = $(wildcard $(SRC_DIR)/*.cpp) \
                   $(wildcard $(SRC_DIR)/*/*.cpp) \
                   $(HTTP_SRCS) \
                   $(CONFIG_SRCS)

# Generate object files list
OBJS              = $(sort $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS)))

# Test executables
TEST_PARSER       = test_parser
TEST_FORM         = test_form
TEST_RESPONSE     = test_response
TEST_HTTP_INT     = test_http_integration
TEST_CGI_UPLOAD   = test_cgi_upload
TEST_CONFIG       = test_config
TEST_CGI_SIMPLE   = test_cgi_simple
TEST_UPLOAD       = test_upload

# Test sources
TEST_PARSER_SRC   = $(TEST_DIR)/unit/test_parser.cpp
TEST_FORM_SRC     = $(TEST_DIR)/unit/test_form_parser.cpp
TEST_RESPONSE_SRC = $(TEST_DIR)/unit/test_response.cpp
TEST_HTTP_INT_SRC = $(TEST_DIR)/integration/http_integration_test.cpp
TEST_CGI_UP_SRC   = $(TEST_DIR)/integration/cgi_upload_test.cpp
TEST_CONFIG_SRC   = $(TEST_DIR)/unit/test_config.cpp
TEST_CGI_SIM_SRC  = $(TEST_DIR)/test_cgi_simple.cpp
TEST_UPLOAD_SRC   = $(TEST_DIR)/unit/test_upload.cpp

# **************************************************************************** #
#                                   RULES                                      #
# **************************************************************************** #

# Default rule: build the server
all: header $(NAME)

header:
	@echo "${BLUE}${BOLD}┌───────────────────────────────────────────┐${RESET}"
	@echo "${BLUE}${BOLD}│           WEBSERV COMPILATION             │${RESET}"
	@echo "${BLUE}${BOLD}└───────────────────────────────────────────┘${RESET}"

# Create object directories
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo "${COLOR_BUILD}➤ Compiling $<${RESET}"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Build the server
$(NAME): $(OBJS)
	@echo "${GREEN}${BOLD}➤ Linking $(NAME)${RESET}"
	@$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	@echo "${GREEN}${BOLD}✓ Build complete: $(NAME)${RESET}"

# Test rules
test: test_header test_unit test_integration test_cgi_simple
	@echo "${GREEN}${BOLD}✓ All tests completed.${RESET}"

test_header:
	@echo "${BLUE}${BOLD}┌───────────────────────────────────────────┐${RESET}"
	@echo "${BLUE}${BOLD}│           WEBSERV TEST SUITE              │${RESET}"
	@echo "${BLUE}${BOLD}└───────────────────────────────────────────┘${RESET}"

test_unit: $(TEST_PARSER) $(TEST_FORM) $(TEST_RESPONSE) $(TEST_CONFIG) $(TEST_UPLOAD)
	@echo "${GREEN}${BOLD}✓ Unit tests completed.${RESET}"

test_integration: $(TEST_HTTP_INT) $(TEST_CGI_UPLOAD)
	@echo "${GREEN}${BOLD}✓ Integration tests completed.${RESET}"

$(TEST_PARSER): 
	@mkdir -p $(OBJ_DIR)
	@echo "${COLOR_TEST}➤ Building parser test${RESET}"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $(HTTP_SRCS) $(TEST_PARSER_SRC) -o $(TEST_PARSER)
	@./$(TEST_PARSER)

$(TEST_FORM): 
	@mkdir -p $(OBJ_DIR)
	@echo "${COLOR_TEST}➤ Building form parser test${RESET}"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $(HTTP_SRCS) $(TEST_FORM_SRC) -o $(TEST_FORM)
	@./$(TEST_FORM)

$(TEST_RESPONSE):
	@mkdir -p $(OBJ_DIR)
	@echo "${COLOR_TEST}➤ Building response test${RESET}"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $(HTTP_SRCS) $(TEST_RESPONSE_SRC) -o $(TEST_RESPONSE)
	@./$(TEST_RESPONSE)
	
$(TEST_HTTP_INT):
	@mkdir -p $(OBJ_DIR)
	@echo "${COLOR_TEST}➤ Building HTTP integration test${RESET}"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $(HTTP_SRCS) $(TEST_HTTP_INT_SRC) -o $(TEST_HTTP_INT)
	@./$(TEST_HTTP_INT)

$(TEST_CGI_UPLOAD):
	@mkdir -p $(OBJ_DIR)
	@echo "${COLOR_TEST}➤ Building CGI upload test${RESET}"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $(HTTP_SRCS) $(TEST_CGI_UP_SRC) -o $(TEST_CGI_UPLOAD)
	@./$(TEST_CGI_UPLOAD)

$(TEST_CONFIG):
	@mkdir -p $(OBJ_DIR)
	@echo "${COLOR_TEST}➤ Building config test${RESET}"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $(CONFIG_SRCS) $(TEST_CONFIG_SRC) -o $(TEST_CONFIG)
	@./$(TEST_CONFIG)

test_cgi_simple:
	@mkdir -p $(OBJ_DIR)
	@echo "${COLOR_TEST}➤ Building simple CGI test${RESET}"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $(HTTP_SRCS) $(TEST_CGI_SIM_SRC) -o $(TEST_CGI_SIMPLE)
	@./$(TEST_CGI_SIMPLE)

$(TEST_UPLOAD):
	@mkdir -p $(OBJ_DIR)
	@echo "${COLOR_TEST}➤ Building upload test${RESET}"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $(HTTP_SRCS) $(TEST_UPLOAD_SRC) -o $(TEST_UPLOAD)
	@./$(TEST_UPLOAD)

# Clean rules
clean:
	@echo "${COLOR_CLEAN}➤ Removing object files${RESET}"
	@rm -rf $(OBJ_DIR)
	@echo "${GREEN}✓ Object files removed${RESET}"

fclean: clean
	@echo "${COLOR_CLEAN}➤ Removing executable and test files${RESET}"
	@rm -f $(NAME)
	@rm -f $(TEST_PARSER)
	@rm -f $(TEST_FORM)
	@rm -f $(TEST_RESPONSE)
	@rm -f $(TEST_HTTP_INT)
	@rm -f $(TEST_CGI_UPLOAD)
	@rm -f $(TEST_CONFIG)
	@rm -f $(TEST_CGI_SIMPLE)
	@rm -f $(TEST_UPLOAD)
	@echo "${GREEN}✓ All generated files removed${RESET}"

re: fclean all

.PHONY: all clean fclean re test test_unit test_integration test_cgi_simple header test_header