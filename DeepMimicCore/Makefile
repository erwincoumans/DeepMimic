BIN=Main
OBJ_DIR = objs
MKDIR = mkdir -p

EIGEN_DIR = /usr/local/include/eigen3
#../../libraries/eigen
BULLET_INC_DIR = /usr/local/include/bullet
#../../libraries/bullet3/src

PYTHON_INC = /usr/local/Cellar/python/3.7.0/Frameworks/Python.framework/Versions/3.7/include/python3.7m
PYTHON_LIB = /usr/local/Cellar/python/3.7.0/Frameworks/Python.framework/Versions/3.7/lib -lpython3.7m
#/usr/lib/ -lpython3.6m

INC = -I./ \
        -IOpenGLWindow/glad \
	-I$(EIGEN_DIR) \
	-I$(BULLET_INC_DIR)


LIBS =  -L. -lOpenGL_Window_gmake_x64_release -lBullet3Common_gmake_x64_release -framework OpenGL -framework Cocoa -lglut -lBulletDynamics -lBulletCollision -lLinearMath

CC = clang++
CFLAGS = -std=c++11 -O3 -Wall -fPIC 

SRC_DIRS = util/ \
        OpenGLWindow/ \
        OpenGLWindow/glad \
	util/json/ \
	anim/ \
	sim/ \
	render/ \
	render/lodepng/ \
	scenes/ \
	scenes/arm/ \
	scenes/particle/ \
	scenes/pendulum/

SRC_FILES = $(BIN).cpp DeepMimicCore.cpp  $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)*.cpp))
OUT_DIRS = $(addprefix $(OBJ_DIR)/, $(SRC_DIRS))
OBJ = $(addprefix $(OBJ_DIR)/, $(patsubst %.cpp, %.o, $(SRC_FILES)))


default: build

build: $(BIN)

print:
	$(OUT_DIRS)

python: $(OUT_DIRS) $(OBJ)
	swig -c++ -python DeepMimicCore.i
	$(CC) -c -g $(CFLAGS) $(INC) $(LIBS) DeepMimicCore.cpp DeepMimicCore_wrap.cxx -I$(PYTHON_INC)
	$(CC) -shared $(CFLAGS) $(OBJ) $(LIBS) DeepMimicCore_wrap.o -o _DeepMimicCore.so -L$(PYTHON_LIB)

$(OBJ_DIR)/%.o: %.cpp
	$(CC) -c -g $(CFLAGS) $(INC) -o $@ $<

$(BIN): $(OUT_DIRS) $(OBJ)
	$(CC) -g $(CFLAGS) $(OBJ) $(LIBS) -o $@

$(OUT_DIRS):
	$(MKDIR) $(OUT_DIRS)
	
clean:
	- rm -f -r $(OUT_DIRS) $(BIN) $(OBJ) $(OBJ_DIR) DeepMimicCore_wrap.cxx _DeepMimicCore.so
