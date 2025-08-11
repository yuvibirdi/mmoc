# CMake generated Testfile for 
# Source directory: /Users/yb/git/mmoc
# Build directory: /Users/yb/git/mmoc/build-clean
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[integration_hello_world]=] "/opt/homebrew/bin/cmake" "-E" "env" "/Users/yb/git/mmoc/build-clean/ccomp" "/Users/yb/git/mmoc/tests/Basic/simple_return.c")
set_tests_properties([=[integration_hello_world]=] PROPERTIES  WORKING_DIRECTORY "/Users/yb/git/mmoc/build-clean" _BACKTRACE_TRIPLES "/Users/yb/git/mmoc/CMakeLists.txt;136;add_test;/Users/yb/git/mmoc/CMakeLists.txt;0;")
