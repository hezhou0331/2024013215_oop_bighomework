CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic -Iinclude
TARGET = project_scheduler

SRCS = \
    main.cpp \
    src/model/Resource.cpp \
    src/model/ResourceAllocation.cpp \
    src/model/Task.cpp \
    src/model/BasicTask.cpp \
    src/model/MilestoneTask.cpp \
    src/model/Dependency.cpp \
    src/model/Project.cpp \
    src/model/ValidationResult.cpp \
    src/model/ProjectValidator.cpp \
    src/model/TaskScheduleInfo.cpp \
    src/model/ScheduleResult.cpp \
    src/model/CPMScheduler.cpp \
    src/model/PPMImporter.cpp \
    src/model/PPMExporter.cpp \
    src/model/ProjectRepository.cpp \
    src/controller/ProjectController.cpp \
    src/view/ConsoleUI.cpp

OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

clean:
	rm -f $(OBJS) $(TARGET) $(TARGET).exe

.PHONY: all clean
