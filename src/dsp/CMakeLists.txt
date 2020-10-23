
include_directories(. ../.. ../debug ../includes ${SDL2_INCLUDE_DIR}) 

add_library(DSP
    dsp.c dsp_core.c dsp_cpu.c dsp_disasm.c
)
