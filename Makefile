CXX = g++ -DGMP_SUPPORT
BUILD_DIR = build

all: gc_debug
debug: CXX += -DGC_INFO -g -pg
gc_debug: CXX += -DGC_INFO -DGC_DEBUG -g -pg

release: $(BUILD_DIR) $(BUILD_DIR)/sonsi
debug: $(BUILD_DIR) $(BUILD_DIR)/sonsi
gc_debug: $(BUILD_DIR) $(BUILD_DIR)/sonsi

_OBJS = main.o \
	parser.o builtin.o \
	model.o eval.o exc.o \
	consts.o types.o gc.o


OBJS = $(patsubst %, $(BUILD_DIR)/%, $(_OBJS))

$(BUILD_DIR)/sonsi: $(OBJS) 
	$(CXX) -o $(BUILD_DIR)/sonsi $^ -lgmp

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o : %.cpp
	$(CXX) -o $@ -c $< -Wall 

clean:
	rm -rf $(BUILD_DIR)

db:
	gdb $(BUILD_DIR)/sonsi

cdb:
	cgdb $(BUILD_DIR)/sonsi

run:
	./$(BUILD_DIR)/sonsi
