
include_directories(. ../.. ../debug ../includes ${SDL2_INCLUDE_DIR}) 

add_library(Dimension
    dimension.cpp nd_mem.cpp nd_nbic.cpp nd_devs.cpp nd_vio.cpp nd_rom.cpp
    i860.cpp i860dis.cpp nd_sdl.cpp
)
