#############################################################
#                                                           #
#                Makefile for tcpsvr                    #
#                  Edit by stanjiang 2012.03.28             #
#############################################################

EXTRA_DIR = ../extra/
PROTOBUF_INC = -I$(EXTRA_DIR)
PROTOBUF_LIB = $(EXTRA_DIR)/google/protobuf/lib
GLOG_INC = -I$(EXTRA_DIR)
GLOG_LIB = $(EXTRA_DIR)/glog/lib

SRC_DIR = ./
OBJ_DIR = ../obj/server/
COMM_DIR = ../comm/
PROTO_DIR = ../proto/include/
PROTO_SRC = $(PROTO_DIR)/cs_proto/

DIRS=$(shell ls  $(SRC_DIR))
MODULE=$(addprefix $(SRC_DIR),$(DIRS))
VPATH=$(MODULE)
MODULE+=$(COMM_DIR) $(PROTO_SRC)

SOURCE=$(wildcard *.cpp)
SOURCE+=$(foreach subdir, ${MODULE}, $(wildcard ${subdir}/*.cpp))
SOURCE+=$(wildcard *.c)
SOURCE+=$(foreach subdir, ${MODULE}, $(wildcard ${subdir}/*.c))

SUB_DIR    =$(shell ls -R|grep ^'./')
SUB_DIR1   =$(patsubst ./%:,%,$(SUB_DIR))
MODULE_INC =$(addprefix -I$(SRC_DIR),$(SUB_DIR1))
MODULE_INC +=-I$(COMM_DIR) -I$(PROTO_DIR) $(PROTOBUF_INC) $(GLOG_INC)

SRC=$(notdir $(SOURCE))
SRCOBJ=$(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SRC)))
OBJS=$(addprefix $(OBJ_DIR), $(SRCOBJ))

INC= $(MODULE_INC)
LIB= $(PROTOBUF_LIB)/libprotobuf.a $(GLOG_LIB)/libglog.a -lrt -lpthread
C_FLAGS= -g -Wall -DNDEBUG

TARGET=$(SRC_DIR)/tcpsvr
INSTALL_TARGET=../bin/

all:$(OBJS)
.PHONY:clean
$(OBJ_DIR)%.o: %.cpp
	@$(CXX) $(INC) $(C_FLAGS) -c $< -o $@ 
	@echo compile   $@
$(OBJ_DIR)%.o: %.c
	@$(CC) $(INC) $(C_FLAGS) -c $< -o $@ 
	@echo compile   $@	
$(TARGET): $(OBJS)
	@$(CXX) $(C_FLAGS) -static  $^ -o $@ $(LIB) 
	@echo -e "Create $@"
	@md5sum $(TARGET)
all: $(TARGET)
install:
	cp -f $(TARGET) $(INSTALL_TARGET)
clean:
	@rm -f $(TARGET) $(OBJS)
	-$(RM) *.o
	-$(RM) *.d	

