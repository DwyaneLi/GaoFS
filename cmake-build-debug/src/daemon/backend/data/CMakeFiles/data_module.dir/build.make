# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/dwyaneli/gaofs

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/dwyaneli/gaofs/cmake-build-debug

# Include any dependencies generated for this target.
include src/daemon/backend/data/CMakeFiles/data_module.dir/depend.make

# Include the progress variables for this target.
include src/daemon/backend/data/CMakeFiles/data_module.dir/progress.make

# Include the compile flags for this target's objects.
include src/daemon/backend/data/CMakeFiles/data_module.dir/flags.make

src/daemon/backend/data/CMakeFiles/data_module.dir/data_module.cpp.o: src/daemon/backend/data/CMakeFiles/data_module.dir/flags.make
src/daemon/backend/data/CMakeFiles/data_module.dir/data_module.cpp.o: ../src/daemon/backend/data/data_module.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dwyaneli/gaofs/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object src/daemon/backend/data/CMakeFiles/data_module.dir/data_module.cpp.o"
	cd /home/dwyaneli/gaofs/cmake-build-debug/src/daemon/backend/data && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/data_module.dir/data_module.cpp.o -c /home/dwyaneli/gaofs/src/daemon/backend/data/data_module.cpp

src/daemon/backend/data/CMakeFiles/data_module.dir/data_module.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/data_module.dir/data_module.cpp.i"
	cd /home/dwyaneli/gaofs/cmake-build-debug/src/daemon/backend/data && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/dwyaneli/gaofs/src/daemon/backend/data/data_module.cpp > CMakeFiles/data_module.dir/data_module.cpp.i

src/daemon/backend/data/CMakeFiles/data_module.dir/data_module.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/data_module.dir/data_module.cpp.s"
	cd /home/dwyaneli/gaofs/cmake-build-debug/src/daemon/backend/data && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/dwyaneli/gaofs/src/daemon/backend/data/data_module.cpp -o CMakeFiles/data_module.dir/data_module.cpp.s

# Object files for target data_module
data_module_OBJECTS = \
"CMakeFiles/data_module.dir/data_module.cpp.o"

# External object files for target data_module
data_module_EXTERNAL_OBJECTS =

src/daemon/backend/data/libdata_module.a: src/daemon/backend/data/CMakeFiles/data_module.dir/data_module.cpp.o
src/daemon/backend/data/libdata_module.a: src/daemon/backend/data/CMakeFiles/data_module.dir/build.make
src/daemon/backend/data/libdata_module.a: src/daemon/backend/data/CMakeFiles/data_module.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/dwyaneli/gaofs/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library libdata_module.a"
	cd /home/dwyaneli/gaofs/cmake-build-debug/src/daemon/backend/data && $(CMAKE_COMMAND) -P CMakeFiles/data_module.dir/cmake_clean_target.cmake
	cd /home/dwyaneli/gaofs/cmake-build-debug/src/daemon/backend/data && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/data_module.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/daemon/backend/data/CMakeFiles/data_module.dir/build: src/daemon/backend/data/libdata_module.a

.PHONY : src/daemon/backend/data/CMakeFiles/data_module.dir/build

src/daemon/backend/data/CMakeFiles/data_module.dir/clean:
	cd /home/dwyaneli/gaofs/cmake-build-debug/src/daemon/backend/data && $(CMAKE_COMMAND) -P CMakeFiles/data_module.dir/cmake_clean.cmake
.PHONY : src/daemon/backend/data/CMakeFiles/data_module.dir/clean

src/daemon/backend/data/CMakeFiles/data_module.dir/depend:
	cd /home/dwyaneli/gaofs/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/dwyaneli/gaofs /home/dwyaneli/gaofs/src/daemon/backend/data /home/dwyaneli/gaofs/cmake-build-debug /home/dwyaneli/gaofs/cmake-build-debug/src/daemon/backend/data /home/dwyaneli/gaofs/cmake-build-debug/src/daemon/backend/data/CMakeFiles/data_module.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/daemon/backend/data/CMakeFiles/data_module.dir/depend

