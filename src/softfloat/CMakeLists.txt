
include_directories(. ../.. ../debug ../includes ${SDL2_INCLUDE_DIR}) 

add_library(SoftFloat
    softfloat.c softfloat_decimal.c softfloat_fpsp.c 
)
