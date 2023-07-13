PROJECT_NAME            = Server
CONFIG = Debug

# File directory
DIR = ./Repository/
CONFIG_DIR          	= $(ARCH)_$(WORDSIZE)bit
BIN_FILE            	= $(PROJECT_NAME)
SRC_DIR 				= $(DIR)src
OBJ_DIR 				= $(DIR)build
BIN_DIR 				= $(DIR)bin
BIN_PATH            	= $(BIN_DIR)/$(BIN_FILE)

PROJECT_DIR             = /home/raman/Desktop/Server
VIMBASDK_DIR			= /opt/Vimba_6_0
MAKE_INCLUDE_DIR        = $(DIR)make_include

include $(MAKE_INCLUDE_DIR)/Common.mk

all: $(BIN_PATH)

include $(MAKE_INCLUDE_DIR)/VimbaC.mk

INCLUDE_DIRS        = -I$(SRC_DIR)

LIBS                = $(VIMBAC_LIBS) \
					  -lrt

DEFINES             =

CFLAGS              = $(COMMON_CFLAGS) \
                      $(VIMBAC_CFLAGS)

OBJ_FILES           = $(OBJ_DIR)/main_server.o \
					  $(OBJ_DIR)/camera_handler.o \
					  $(OBJ_DIR)/pruebas_server.o

DEPENDENCIES        = VimbaC

$(OBJ_DIR)/%.o: $(COMMON_DIR)/%.c $(OBJ_DIR)
	$(CXX) -c $(INCLUDE_DIRS) $(DEFINES) $(CFLAGS) -o $@ $<

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(OBJ_DIR)
	$(CXX) -c $(INCLUDE_DIRS) $(DEFINES) $(CFLAGS) -o $@ $<

$(BIN_PATH): $(DEPENDENCIES) $(OBJ_FILES) $(BIN_DIR)
	$(CXX) $(ARCH_CFLAGS) -o $(BIN_PATH) $(OBJ_FILES) $(LIBS) -Wl,-rpath,'$$ORIGIN'

clean:
	$(RM) -r -f $(OBJ_FILES)
	$(RM) -r -f $(BIN_PATH)
	$(RM) -f $(BIN_DIR)/libVimbaC.so

$(OBJ_DIR):
	$(MKDIR) -p $(OBJ_DIR)

$(BIN_DIR):
	$(MKDIR) -p $(BIN_DIR)