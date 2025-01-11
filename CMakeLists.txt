# cmake-format: off
# /CMakeLists.txt -*-makefile-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# cmake-format: on
# note to self: CXX=/opt/llvm-19.1.6/bin/clang++ LDFLAGS=-fuse-ld=lld mk

cmake_minimum_required(VERSION 3.25...3.31)

project(stdexec_examples VERSION 0.0.1 LANGUAGES CXX)

add_subdirectory(beman-task)
add_subdirectory(stdexec)
add_subdirectory(libunifex)

list(
  APPEND
  BEMAN_EXAMPLES
  demo-into_optional
)
list(
  APPEND
  xBEMAN_EXAMPLES
  beman-affinity
  beman-alloc
  beman-loop
  beman-return
  beman-stopped
  demo-co_await-result
  demo-affinity
  demo-alloc
  demo-error
  demo-friendly
  demo-hello
  demo-query
  demo-result_example
  demo-stop
)

list(
  APPEND
  STDEXEC_EXAMPLES
  stdexec-affinity
  stdexec-alloc
  stdexec-loop
  stdexec-return
  stdexec-stopped
)

list(
  APPEND
  UNIFEX_EXAMPLES
  unifex-affinity
  unifex-alloc
  unifex-loop
  unifex-return
  unifex-stopped
)

foreach(EXAMPLE ${BEMAN_EXAMPLES})
  set(EXAMPLE_TARGET ${EXAMPLE})
  add_executable(${EXAMPLE_TARGET})
  target_sources(${EXAMPLE_TARGET} PRIVATE ${EXAMPLE}.cpp)
  target_link_libraries(${EXAMPLE_TARGET} PRIVATE beman::task)
endforeach()

foreach(EXAMPLE ${STDEXEC_EXAMPLES})
  set(EXAMPLE_TARGET ${EXAMPLE})
  add_executable(${EXAMPLE_TARGET})
  target_sources(${EXAMPLE_TARGET} PRIVATE ${EXAMPLE}.cpp)
  target_link_libraries(${EXAMPLE_TARGET} PRIVATE STDEXEC::stdexec)
endforeach()

foreach(EXAMPLE ${UNIFEX_EXAMPLES})
  set(EXAMPLE_TARGET ${EXAMPLE})
  add_executable(${EXAMPLE_TARGET})
  target_sources(${EXAMPLE_TARGET} PRIVATE ${EXAMPLE}.cpp)
  target_link_libraries(${EXAMPLE_TARGET} PRIVATE unifex)
endforeach()

