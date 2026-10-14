# Developer reference tools built from the original EASy68K sources.
# Off by default; enable with -DBUILD_REFERENCE_TOOLS=ON.
set(EASY68K_ORIGINAL_SOURCE_DIR "${CMAKE_SOURCE_DIR}/../Easy68k"
    CACHE PATH "Path to a checkout of the original EASy68K sources")
add_subdirectory(edit68k)
add_subdirectory(sim68k)
