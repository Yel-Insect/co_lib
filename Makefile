# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Default target executed when no arguments are given to make.
default_target: all
.PHONY : default_target

# Allow only one "make -f Makefile2" at a time, but pass parallelism.
.NOTPARALLEL:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
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
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/beautiful/桌面/server/sylar

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/beautiful/桌面/server/sylar

#=============================================================================
# Targets provided globally by CMake.

# Special rule for the target edit_cache
edit_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "No interactive CMake dialog available..."
	/usr/bin/cmake -E echo No\ interactive\ CMake\ dialog\ available.
.PHONY : edit_cache

# Special rule for the target edit_cache
edit_cache/fast: edit_cache
.PHONY : edit_cache/fast

# Special rule for the target rebuild_cache
rebuild_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Running CMake to regenerate build system..."
	/usr/bin/cmake --regenerate-during-build -S$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR)
.PHONY : rebuild_cache

# Special rule for the target rebuild_cache
rebuild_cache/fast: rebuild_cache
.PHONY : rebuild_cache/fast

# The main all target
all: cmake_check_build_system
	$(CMAKE_COMMAND) -E cmake_progress_start /home/beautiful/桌面/server/sylar/CMakeFiles /home/beautiful/桌面/server/sylar//CMakeFiles/progress.marks
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 all
	$(CMAKE_COMMAND) -E cmake_progress_start /home/beautiful/桌面/server/sylar/CMakeFiles 0
.PHONY : all

# The main clean target
clean:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 clean
.PHONY : clean

# The main clean target
clean/fast: clean
.PHONY : clean/fast

# Prepare targets for installation.
preinstall: all
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall

# Prepare targets for installation.
preinstall/fast:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall/fast

# clear depends
depend:
	$(CMAKE_COMMAND) -S$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 1
.PHONY : depend

#=============================================================================
# Target rules for targets named main

# Build rule for target.
main: cmake_check_build_system
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 main
.PHONY : main

# fast build rule for target.
main/fast:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/build
.PHONY : main/fast

config.o: config.cc.o
.PHONY : config.o

# target to build an object file
config.cc.o:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/config.cc.o
.PHONY : config.cc.o

config.i: config.cc.i
.PHONY : config.i

# target to preprocess a source file
config.cc.i:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/config.cc.i
.PHONY : config.cc.i

config.s: config.cc.s
.PHONY : config.s

# target to generate assembly for a file
config.cc.s:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/config.cc.s
.PHONY : config.cc.s

fd_manager.o: fd_manager.cc.o
.PHONY : fd_manager.o

# target to build an object file
fd_manager.cc.o:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/fd_manager.cc.o
.PHONY : fd_manager.cc.o

fd_manager.i: fd_manager.cc.i
.PHONY : fd_manager.i

# target to preprocess a source file
fd_manager.cc.i:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/fd_manager.cc.i
.PHONY : fd_manager.cc.i

fd_manager.s: fd_manager.cc.s
.PHONY : fd_manager.s

# target to generate assembly for a file
fd_manager.cc.s:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/fd_manager.cc.s
.PHONY : fd_manager.cc.s

fiber.o: fiber.cc.o
.PHONY : fiber.o

# target to build an object file
fiber.cc.o:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/fiber.cc.o
.PHONY : fiber.cc.o

fiber.i: fiber.cc.i
.PHONY : fiber.i

# target to preprocess a source file
fiber.cc.i:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/fiber.cc.i
.PHONY : fiber.cc.i

fiber.s: fiber.cc.s
.PHONY : fiber.s

# target to generate assembly for a file
fiber.cc.s:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/fiber.cc.s
.PHONY : fiber.cc.s

hook.o: hook.cc.o
.PHONY : hook.o

# target to build an object file
hook.cc.o:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/hook.cc.o
.PHONY : hook.cc.o

hook.i: hook.cc.i
.PHONY : hook.i

# target to preprocess a source file
hook.cc.i:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/hook.cc.i
.PHONY : hook.cc.i

hook.s: hook.cc.s
.PHONY : hook.s

# target to generate assembly for a file
hook.cc.s:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/hook.cc.s
.PHONY : hook.cc.s

iomanager.o: iomanager.cc.o
.PHONY : iomanager.o

# target to build an object file
iomanager.cc.o:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/iomanager.cc.o
.PHONY : iomanager.cc.o

iomanager.i: iomanager.cc.i
.PHONY : iomanager.i

# target to preprocess a source file
iomanager.cc.i:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/iomanager.cc.i
.PHONY : iomanager.cc.i

iomanager.s: iomanager.cc.s
.PHONY : iomanager.s

# target to generate assembly for a file
iomanager.cc.s:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/iomanager.cc.s
.PHONY : iomanager.cc.s

log.o: log.cc.o
.PHONY : log.o

# target to build an object file
log.cc.o:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/log.cc.o
.PHONY : log.cc.o

log.i: log.cc.i
.PHONY : log.i

# target to preprocess a source file
log.cc.i:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/log.cc.i
.PHONY : log.cc.i

log.s: log.cc.s
.PHONY : log.s

# target to generate assembly for a file
log.cc.s:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/log.cc.s
.PHONY : log.cc.s

main.o: main.cc.o
.PHONY : main.o

# target to build an object file
main.cc.o:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/main.cc.o
.PHONY : main.cc.o

main.i: main.cc.i
.PHONY : main.i

# target to preprocess a source file
main.cc.i:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/main.cc.i
.PHONY : main.cc.i

main.s: main.cc.s
.PHONY : main.s

# target to generate assembly for a file
main.cc.s:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/main.cc.s
.PHONY : main.cc.s

scheduler.o: scheduler.cc.o
.PHONY : scheduler.o

# target to build an object file
scheduler.cc.o:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/scheduler.cc.o
.PHONY : scheduler.cc.o

scheduler.i: scheduler.cc.i
.PHONY : scheduler.i

# target to preprocess a source file
scheduler.cc.i:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/scheduler.cc.i
.PHONY : scheduler.cc.i

scheduler.s: scheduler.cc.s
.PHONY : scheduler.s

# target to generate assembly for a file
scheduler.cc.s:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/scheduler.cc.s
.PHONY : scheduler.cc.s

thread.o: thread.cc.o
.PHONY : thread.o

# target to build an object file
thread.cc.o:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/thread.cc.o
.PHONY : thread.cc.o

thread.i: thread.cc.i
.PHONY : thread.i

# target to preprocess a source file
thread.cc.i:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/thread.cc.i
.PHONY : thread.cc.i

thread.s: thread.cc.s
.PHONY : thread.s

# target to generate assembly for a file
thread.cc.s:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/thread.cc.s
.PHONY : thread.cc.s

timer.o: timer.cc.o
.PHONY : timer.o

# target to build an object file
timer.cc.o:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/timer.cc.o
.PHONY : timer.cc.o

timer.i: timer.cc.i
.PHONY : timer.i

# target to preprocess a source file
timer.cc.i:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/timer.cc.i
.PHONY : timer.cc.i

timer.s: timer.cc.s
.PHONY : timer.s

# target to generate assembly for a file
timer.cc.s:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/timer.cc.s
.PHONY : timer.cc.s

util.o: util.cc.o
.PHONY : util.o

# target to build an object file
util.cc.o:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/util.cc.o
.PHONY : util.cc.o

util.i: util.cc.i
.PHONY : util.i

# target to preprocess a source file
util.cc.i:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/util.cc.i
.PHONY : util.cc.i

util.s: util.cc.s
.PHONY : util.s

# target to generate assembly for a file
util.cc.s:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/util.cc.s
.PHONY : util.cc.s

# Help Target
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... clean"
	@echo "... depend"
	@echo "... edit_cache"
	@echo "... rebuild_cache"
	@echo "... main"
	@echo "... config.o"
	@echo "... config.i"
	@echo "... config.s"
	@echo "... fd_manager.o"
	@echo "... fd_manager.i"
	@echo "... fd_manager.s"
	@echo "... fiber.o"
	@echo "... fiber.i"
	@echo "... fiber.s"
	@echo "... hook.o"
	@echo "... hook.i"
	@echo "... hook.s"
	@echo "... iomanager.o"
	@echo "... iomanager.i"
	@echo "... iomanager.s"
	@echo "... log.o"
	@echo "... log.i"
	@echo "... log.s"
	@echo "... main.o"
	@echo "... main.i"
	@echo "... main.s"
	@echo "... scheduler.o"
	@echo "... scheduler.i"
	@echo "... scheduler.s"
	@echo "... thread.o"
	@echo "... thread.i"
	@echo "... thread.s"
	@echo "... timer.o"
	@echo "... timer.i"
	@echo "... timer.s"
	@echo "... util.o"
	@echo "... util.i"
	@echo "... util.s"
.PHONY : help



#=============================================================================
# Special targets to cleanup operation of make.

# Special rule to run CMake to check the build system integrity.
# No rule that depends on this can have commands that come from listfiles
# because they might be regenerated.
cmake_check_build_system:
	$(CMAKE_COMMAND) -S$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 0
.PHONY : cmake_check_build_system

