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
include src/common/CMakeFiles/distributor.dir/depend.make

# Include the progress variables for this target.
include src/common/CMakeFiles/distributor.dir/progress.make

# Include the compile flags for this target's objects.
include src/common/CMakeFiles/distributor.dir/flags.make

src/common/CMakeFiles/distributor.dir/rpc/distributor.cpp.o: src/common/CMakeFiles/distributor.dir/flags.make
src/common/CMakeFiles/distributor.dir/rpc/distributor.cpp.o: ../src/common/rpc/distributor.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dwyaneli/gaofs/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object src/common/CMakeFiles/distributor.dir/rpc/distributor.cpp.o"
	cd /home/dwyaneli/gaofs/cmake-build-debug/src/common && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/distributor.dir/rpc/distributor.cpp.o -c /home/dwyaneli/gaofs/src/common/rpc/distributor.cpp

src/common/CMakeFiles/distributor.dir/rpc/distributor.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/distributor.dir/rpc/distributor.cpp.i"
	cd /home/dwyaneli/gaofs/cmake-build-debug/src/common && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/dwyaneli/gaofs/src/common/rpc/distributor.cpp > CMakeFiles/distributor.dir/rpc/distributor.cpp.i

src/common/CMakeFiles/distributor.dir/rpc/distributor.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/distributor.dir/rpc/distributor.cpp.s"
	cd /home/dwyaneli/gaofs/cmake-build-debug/src/common && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/dwyaneli/gaofs/src/common/rpc/distributor.cpp -o CMakeFiles/distributor.dir/rpc/distributor.cpp.s

# Object files for target distributor
distributor_OBJECTS = \
"CMakeFiles/distributor.dir/rpc/distributor.cpp.o"

# External object files for target distributor
distributor_EXTERNAL_OBJECTS =

src/common/libdistributor.a: src/common/CMakeFiles/distributor.dir/rpc/distributor.cpp.o
src/common/libdistributor.a: src/common/CMakeFiles/distributor.dir/build.make
src/common/libdistributor.a: src/common/CMakeFiles/distributor.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/dwyaneli/gaofs/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library libdistributor.a"
	cd /home/dwyaneli/gaofs/cmake-build-debug/src/common && $(CMAKE_COMMAND) -P CMakeFiles/distributor.dir/cmake_clean_target.cmake
	cd /home/dwyaneli/gaofs/cmake-build-debug/src/common && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/distributor.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/common/CMakeFiles/distributor.dir/build: src/common/libdistributor.a

.PHONY : src/common/CMakeFiles/distributor.dir/build

src/common/CMakeFiles/distributor.dir/clean:
	cd /home/dwyaneli/gaofs/cmake-build-debug/src/common && $(CMAKE_COMMAND) -P CMakeFiles/distributor.dir/cmake_clean.cmake
.PHONY : src/common/CMakeFiles/distributor.dir/clean

src/common/CMakeFiles/distributor.dir/depend:
	cd /home/dwyaneli/gaofs/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/dwyaneli/gaofs /home/dwyaneli/gaofs/src/common /home/dwyaneli/gaofs/cmake-build-debug /home/dwyaneli/gaofs/cmake-build-debug/src/common /home/dwyaneli/gaofs/cmake-build-debug/src/common/CMakeFiles/distributor.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/common/CMakeFiles/distributor.dir/depend

