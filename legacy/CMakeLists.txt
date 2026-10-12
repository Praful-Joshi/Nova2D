# Legacy Engine, the Tag showcase game and their tests.
# Built locally only: CI configures with -DNOVA2D_BUILD_LEGACY=OFF.

find_package(cppzmq  REQUIRED)
find_package(Boost   REQUIRED)   # headers only (property_tree)
find_package(Threads REQUIRED)

add_subdirectory(engine)
add_subdirectory(tag)
add_subdirectory(tests)
