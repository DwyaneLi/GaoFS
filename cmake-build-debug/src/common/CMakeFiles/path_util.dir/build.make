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
include src/common/CMakeFiles/path_util.dir/depend.make

# Include the progress variables for this target.
include src/common/CMakeFiles/path_util.dir/progress.make

# Include the compile flags for this target's objects.
include src/common/CMakeFiles/path_util.dir/flags.make

src/common/CMakeFiles/path_util.dir/path_util.cpp.o: src/common/CMakeFiles/path_util.dir/flags.make
src/common/CMakeFiles/path_util.dir/path_util.cpp.o: ../src/common/path_util.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dwyaneli/gaofs/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object src/common/CMakeFiles/path_util.dir/path_util.cpp.o"
	cd /home/dwyaneli/gaofs/cmake-build-debug/src/common && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/path_util.dir/path_util.cpp.o -c /home/dwyaneli/gaofs/src/common/path_util.cpp

src/common/CMakeFiles/path_util.dir/path_util.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/path_util.dir/path_util.cpp.i"
	cd /home/dwyaneli/gaofs/cmake-build-debug/src/common && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/dwyaneli/gaofs/src/common/path_util.cpp > CMakeFiles/path_util.dir/path_util.cpp.i

src/common/CMakeFiles/path_util.dir/path_util.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/path_util.dir/path_util.cpp.s"
	cd /home/dwyaneli/gaofs/cmake-build-debug/src/common && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/dwyaneli/gaofs/src/common/path_util.cpp -o CMakeFiles/path_util.dir/path_util.cpp.s

# Object files for target path_util
path_util_OBJECTS = \
"CMakeFiles/path_util.dir/path_util.cpp.o"

# External object files for target path_util
path_util_EXTERNAL_OBJECTS =

src/common/libpath_util.a: src/common/CMakeFiles/path_util.dir/path_util.cpp.o
src/common/libpath_util.a: src/common/CMakeFiles/path_util.dir/build.make
src/common/libpath_util.a: src/common/CMakeFiles/path_util.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/dwyaneli/gaofs/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library libpath_util.a"
	cd /home/dwyaneli/gaofs/cmake-build-debug/src/common && $(CMAKE_COMMAND) -P CMakeFiles/path_util.dir/cmake_clean_target.cmake
	cd /home/dwyaneli/gaofs/cmake-build-debug/src/common && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/path_util.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/common/CMakeFiles/path_util.dir/build: src/common/libpath_util.a

.PHONY : src/common/CMakeFiles/path_util.dir/build

src/common/CMakeFiles/path_util.dir/clean:
	cd /home/dwyaneli/gaofs/cmake-build-debug/src/common && $(CMAKE_COMMAND) -P CMakeFiles/path_util.dir/cmake_clean.cmake
.PHONY : src/common/CMakeFiles/path_util.dir/clean

src/common/CMakeFiles/path_util.dir/depend:
	cd /home/dwyaneli/gaofs/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/dwyaneli/gaofs /home/dwyaneli/gaofs/src/common /home/dwyaneli/gaofs/cmake-build-debug /home/dwyaneli/gaofs/cmake-build-debug/src/common /home/dwyaneli/gaofs/cmake-build-debug/src/common/CMakeFiles/path_util.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/common/CMakeFiles/path_util.dir/depend

