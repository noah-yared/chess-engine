CXX := g++
IDIR := include
TEST_IDIR := tests/boards/pieceBoards
ODIR := build
SRCDIR := src
LIBDIR := libs
DEPDIR := $(ODIR)/deps
TESTDIR := tests
TARGET := engine
TEST_TARGET := test

# specify search paths
vpath %.cpp $(SRCDIR)
vpath %.hpp $(IDIR)
vpath %.h $(IDIR)
vpath %.o $(ODIR)

SOURCES := $(wildcard $(SRCDIR)/*.cpp)
OBJECTS := $(SOURCES:$(SRCDIR)/%.cpp=$(ODIR)/%.o)

TESTSOURCES := $(wildcard $(TESTDIR)/*.cpp)
TESTOBJECTS := $(TESTSOURCES:$(TESTDIR)/%.cpp=$(ODIR)/test_%.o)

# LIBS := $(LIBDIR)/Crow
CXXFLAGS := -std=c++17 -I$(IDIR) -Wall
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

$(TARGET): $(OBJECTS)
	@echo "Building..."
	@$(CXX) -o $(ODIR)/$(TARGET) $(OBJECTS) $(CXXFLAGS)
	@echo "Finished build"

# compiling test objects
$(ODIR)/test_%.o: $(TESTDIR)/%.cpp $(DEPDIR)/%.d | $(ODIR)
	@$(CXX) $(DEPFLAGS) $(CXXFLAGS) -c $< -o $@ -I$(TEST_IDIR) -g

# compiling source objects
$(ODIR)/%.o: $(SRCDIR)/%.cpp $(DEPDIR)/%.d | $(ODIR) $(DEPDIR)
	@$(CXX) $(DEPFLAGS) $(CXXFLAGS) -c $< -o $@ -g

$(ODIR): # set up build directory
	@mkdir -p $@

$(DEPDIR): # set up dependencies directory
	@mkdir -p $@

TEST_DEPFILES = $(TESTSOURCES:$(TESTDIR)/%.cpp=$(DEPDIR)/%.d)
DEPFILES = $(SOURCES:$(SRCDIR)/%.cpp=$(DEPDIR)/%.d)

$(TEST_DEPFILES):

$(DEPFILES):

include $(wildcard $(DEPFILES))

# run tests (use -O0)
.PHONY: tests
tests: $(OBJECTS) $(TESTOBJECTS)
	@echo "Running tests"
	# add in new include directory 
	@$(CXX) -o $(ODIR)/$(TEST_TARGET) $^ $(CXXFLAGS) -I$(TEST_IDIR) -O0 -g
	@./$(ODIR)/$(TEST_TARGET)

# get debugger info (use -O0)
.PHONY: debug
debug: $(OBJECTS)
	@echo "Building in debug mode..."
	@$(MAKE) CXXFLAGS="$(CXXFLAGS) -g -O0"
	@./$(ODIR)/$(TARGET)

# get google perf test data (use -O0)
.PHONY: profile
profile:
	@echo "Building in profiling mode..."
	# @$(MAKE) CXXFLAGS="$(CXXFLAGS) -g -pg -O0"

# run performance test (use -O2)
.PHONY: perft
perft:
	@echo "Building in performance mode..."
	@$(MAKE) CXXFLAGS="$(CXXFLAGS) -O2"

.PHONY: run
run: $(TARGET)
	@echo "Executing executable..."
	./$(ODIR)/$(TARGET)

.PHONY: clean
clean:
	@echo "Cleaning $(ODIR) directory..."
	@rm -rf $(ODIR)