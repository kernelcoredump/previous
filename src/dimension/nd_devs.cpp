#include <stdio.h>
#include <limits.h>
#include <stdlib.h>

#include "main.h"
#include "configuration.h"
#include "m68000.h"
#include "sysdeps.h"
#include "dimension.hpp"
#include "nd_mem.hpp"
#include "nd_nbic.hpp"
#include "i860cfg.h"
#include "nd_sdl.hpp"
#include "ramdac.h"
#include "host.h"
#include "log.h"

/* --------- NEXTDIMENSION DEVICES ---------- */

/* Device registers */

/* Memory controller */
#define ND_MC_CSR0          0xFF800000
#define ND_MC_CSR1          0xFF800010
#define ND_MC_CSR2          0xFF800020
#define ND_MC_SID           0xFF800030

#define ND_MC_DMA_CSR       0xFF801000

#define ND_MC_VRAM_TIMING	0xFF802000
#define ND_MC_DRAM_SIZE		0xFF803000

/* CSR bits */
#define CSR0_i860PIN_RESET  0x00000001
#define CSR0_i860PIN_CS8    0x00000002
#define CSR0_i860_IMASK     0x00000004
#define CSR0_i860_INT       0x00000008
#define CSR0_BE_IMASK       0x00000010
#define CSR0_BE_INT         0x00000020
#define CSR0_VBL_IMASK      0x00000040
#define CSR0_VBL_INT        0x00000080
#define CSR0_VBLANK         0x00000100 /* ro */
#define CSR0_VIOVBL_IMASK   0x00000200
#define CSR0_VIOVBL_INT     0x00000400
#define CSR0_VIOBLANK       0x00000800 /* ro */
#define CSR0_i860_CACHE_EN  0x00001000

#define CSR1_CPU_INT        0x00000001

#define CSR2_GLOBAL_ACCESS  0x00000001

#define SID_SID_MASK        0x0000000F
#define SID_STEP_MASK       0x000000F0

#define CSRDMA_VISIBLE_EN   0x00000001
#define CSRDMA_BLANKED_EN   0x00000002
#define CSRDMA_READ_EN      0x00000004

#define CSRVRAM_VBLANK      0x00000001
#define CSRVRAM_60HZ        0x00000002
#define CSRVRAM_EXT_SYNC    0x00000004

#define CSRDRAM_4MBIT       0x00000001

#define DP_IIC_MORE 0x20000000
#define DP_IIC_BUSY 0x80000000

#define DP_CSR_MASK_DIS     0x01
#define DP_CSR_NTSC         0x02 /* 0 = NTSC, 1 = PAL */
#define DP_CSR_JPEG_MASK    0xFC

MC::MC(NextDimension* nd) : nd(nd) {}

void MC::init(void) {
    csr0          = CSR0_i860PIN_CS8;
    csr1          = 0;
    csr2          = 0;
    sid           = nd->slot|(ND_STEP<<4);
    dma_csr       = 0;
    dma_start     = 0;
    dma_width     = 0;
    dma_pstart    = 0;
    dma_pwidth    = 0;
    dma_sstart    = 0;
    dma_swidth    = 0;
    dma_bsstart   = 0;
    dma_bswidth   = 0;
    dma_top       = 0;
    dma_bottom    = 0;
    dma_line_a    = 0;
    dma_curr_a    = 0;
    dma_scurr_a   = 0;
    dma_out_a     = 0;
    vram          = 0;
    dram          = 0;
}

DP::DP(NextDimension* nd) : nd(nd) {}

void DP::init(void) {
    iic_msgsz     = 0;
    iic_addr      = 0;
    csr           = 0;
    alpha         = 0;
    dma           = 0;
    cpu_x         = 0xc;
    cpu_y         = 0xc;
    dma_x         = 0xd;
    dma_y         = 0xd;
    iic_stat_addr = 0;
    iic_data      = 0;
}


static const char* ND_CSR0_BITS[] = {
    "i860PIN_RESET",   "i860PIN_CS8",     "i860_IMASK",  "i860_INT",
    "BE_IMASK",        "BE_INT",          "VBL_IMASK",   "VBL_INT",
    "VBLANK",          "VIOVBL_IMASK",    "VIOVBL_INT",  "VIOBLANK",
    "i860_CACHE_EN",   "00002000",        "00004000",    "00008000",
    "00010000",        "00020000",        "00040000",    "00080000",
    "00100000",        "00200000",        "00400000",    "00800000",
    "01000000",        "02000000",        "04000000",    "08000000",
    "10000000",        "20000000",        "40000000",    "80000000",
};

static const char* ND_CSR1_BITS[] = {
    "CPU_INT",         "00000002",        "00000004",    "00000008",
    "00000010",        "00000020",        "00000040",    "00000080",
    "00000100",        "00000200",        "00000400",    "00000800",
    "00001000",        "00002000",        "00004000",    "00008000",
    "00010000",        "00020000",        "00040000",    "00080000",
    "00100000",        "00200000",        "00400000",    "00800000",
    "01000000",        "02000000",        "04000000",    "08000000",
    "10000000",        "20000000",        "40000000",    "80000000",
};

static const char* ND_CSR2_BITS[] = {
    "GLOBAL_ACCESS",   "00000002",        "00000004",    "00000008",
    "00000010",        "00000020",        "00000040",    "00000080",
    "00000100",        "00000200",        "00000400",    "00000800",
    "00001000",        "00002000",        "00004000",    "00008000",
    "00010000",        "00020000",        "00040000",    "00080000",
    "00100000",        "00200000",        "00400000",    "00800000",
    "01000000",        "02000000",        "04000000",    "08000000",
    "10000000",        "20000000",        "40000000",    "80000000",
};

static const char* ND_DMA_CSR_BITS[] = {
    "VISIBLE_EN",      "BLANKED_EN",      "READ_EN",    "00000008",
    "00000010",        "00000020",        "00000040",    "00000080",
    "00000100",        "00000200",        "00000400",    "00000800",
    "00001000",        "00002000",        "00004000",    "00008000",
    "00010000",        "00020000",        "00040000",    "00080000",
    "00100000",        "00200000",        "00400000",    "00800000",
    "01000000",        "02000000",        "04000000",    "08000000",
    "10000000",        "20000000",        "40000000",    "80000000",
};

static const char* ND_VRAM_BITS[] = {
    "VBLANK",          "60HZ",            "EXT_SYNC",    "00000008",
    "00000010",        "00000020",        "00000040",    "00000080",
    "00000100",        "00000200",        "00000400",    "00000800",
    "00001000",        "00002000",        "00004000",    "00008000",
    "00010000",        "00020000",        "00040000",    "00080000",
    "00100000",        "00200000",        "00400000",    "00800000",
    "01000000",        "02000000",        "04000000",    "08000000",
    "10000000",        "20000000",        "40000000",    "80000000",
};

static const char* ND_DRAM_BITS[] = {
    "4MBIT",           "00000002",        "00000004",    "00000008",
    "00000010",        "00000020",        "00000040",    "00000080",
    "00000100",        "00000200",        "00000400",    "00000800",
    "00001000",        "00002000",        "00004000",    "00008000",
    "00010000",        "00020000",        "00040000",    "00080000",
    "00100000",        "00200000",        "00400000",    "00800000",
    "01000000",        "02000000",        "04000000",    "08000000",
    "10000000",        "20000000",        "40000000",    "80000000",
};

static const char* decodeBits(const char** bits, uae_u32 val) {
    static char buffer[512];
    char*       result = buffer;
    
    if(bits) {
        *result = 0;
        for(int i = 0; i < 32; i++) {
            if(val & (1 << i)) {
                const char* str = bits[i];
                while(*str) *result++ = *str++;
                *result++ = '|';
            }
        }
        if(result != buffer)
            *--result = 0;
    }
    else
        sprintf(buffer, "%08X", val);
    return buffer;
}

static const char* MC_RD_FORMAT   = "[ND] Memory controller %s read %08X at %08X";
static const char* MC_RD_FORMAT_S = "[ND] Memory controller %s read (%s) at %08X";

Uint32 MC::read(Uint32 addr) {
	switch (addr&0x3FFF) {
		case 0x0000:
            Log_Printf(ND_LOG_IO_RD, MC_RD_FORMAT_S,"csr0", decodeBits(ND_CSR0_BITS, csr0),addr);
			return csr0;
		case 0x0010:
            Log_Printf(ND_LOG_IO_RD, MC_RD_FORMAT_S,"csr1", decodeBits(ND_CSR1_BITS, csr1),addr);
			return csr1;
		case 0x0020:
            Log_Printf(ND_LOG_IO_RD, MC_RD_FORMAT_S,"csr2", decodeBits(ND_CSR2_BITS, csr2),addr);
			return csr2;
		case 0x0030:
            Log_Printf(ND_LOG_IO_RD, MC_RD_FORMAT,"sid", sid,addr);
			return sid;
		case 0x1000:
            Log_Printf(ND_LOG_IO_RD, MC_RD_FORMAT_S,"dma_csr", decodeBits(ND_DMA_CSR_BITS, dma_csr),addr);
            return dma_csr;
        case 0x1010:
            Log_Printf(ND_LOG_IO_RD, MC_RD_FORMAT,"dma_start", dma_start,addr);
            return dma_start;
        case 0x1020:
            Log_Printf(ND_LOG_IO_RD, MC_RD_FORMAT,"dma_width", dma_width,addr);
            return dma_width;
        case 0x1030:
            Log_Printf(ND_LOG_IO_RD, MC_RD_FORMAT,"dma_pstart", dma_pstart,addr);
            return dma_pstart;
        case 0x1040:
            Log_Printf(ND_LOG_IO_RD, MC_RD_FORMAT,"dma_pwidth", dma_pwidth,addr);
            return dma_pwidth;
        case 0x1050:
            Log_Printf(ND_LOG_IO_RD, MC_RD_FORMAT,"dma_sstart", dma_sstart,addr);
            return dma_sstart;
        case 0x1060:
            Log_Printf(ND_LOG_IO_RD, MC_RD_FORMAT,"dma_swidth", dma_swidth,addr);
            return dma_swidth;
        case 0x1070:
            Log_Printf(ND_LOG_IO_RD, MC_RD_FORMAT,"dma_bsstart", dma_bsstart,addr);
            return dma_bsstart;
        case 0x1080:
            Log_Printf(ND_LOG_IO_RD, MC_RD_FORMAT,"dma_bswidth", dma_bswidth,addr);
            return dma_bswidth;
        case 0x1090:
            Log_Printf(ND_LOG_IO_RD, MC_RD_FORMAT,"dma_top", dma_top,addr);
            return dma_top;
        case 0x10A0:
            Log_Printf(ND_LOG_IO_RD, MC_RD_FORMAT,"dma_bottom", dma_bottom,addr);
            return dma_bottom;
        case 0x10B0:
            Log_Printf(ND_LOG_IO_RD, MC_RD_FORMAT,"dma_line_a", dma_line_a,addr);
            return dma_line_a;
        case 0x10C0:
            Log_Printf(ND_LOG_IO_RD, MC_RD_FORMAT,"dma_curr_a", dma_curr_a,addr);
            return dma_curr_a;
        case 0x10D0:
            Log_Printf(ND_LOG_IO_RD, MC_RD_FORMAT,"dma_line_a", dma_line_a,addr);
            return dma_line_a;
        case 0x10E0:
            Log_Printf(ND_LOG_IO_RD, MC_RD_FORMAT,"dma_scurr_a", dma_scurr_a,addr);
            return dma_scurr_a;
        case 0x10F0:
            Log_Printf(ND_LOG_IO_RD, MC_RD_FORMAT,"dma_out_a", dma_out_a,addr);
            return dma_out_a;
		case 0x2000:
            Log_Printf(ND_LOG_IO_RD, MC_RD_FORMAT_S,"vram", decodeBits(ND_VRAM_BITS, vram),addr);
            return vram;
		case 0x3000:
            Log_Printf(ND_LOG_IO_RD, MC_RD_FORMAT_S,"dram", decodeBits(ND_DRAM_BITS, dram),addr);
            return dram;
		default:
			Log_Printf(LOG_WARN, "[ND] Memory controller UNKNOWN read at %08X",addr);
            break;
	}
	return 0;
}

static const char* MC_WR_FORMAT   = "[ND] Memory controller %s write %08X at %08X";
static const char* MC_WR_FORMAT_S = "[ND] Memory controller %s write (%s) at %08X";

void MC::write(Uint32 addr, Uint32 val) {
    switch (addr&0x3FFF) {
        case 0x0000:
            Log_Printf(ND_LOG_IO_WR, MC_WR_FORMAT_S,"csr0", decodeBits(ND_CSR0_BITS, val), addr);
            if(val & CSR0_i860PIN_RESET) {
                nd->send_msg(MSG_I860_RESET);
                val &= ~CSR0_i860PIN_RESET;
            }
            if ((val & CSR0_i860_INT) && (val & CSR0_i860_IMASK))
                nd->send_msg(MSG_INTR);

            if((val & CSR0_BE_INT) && (val & CSR0_BE_IMASK))
                nd->send_msg(MSG_INTR);

            csr0 = val;
            break;
        case 0x0010:
            Log_Printf(ND_LOG_IO_WR, MC_WR_FORMAT_S,"csr1", decodeBits(ND_CSR1_BITS, val),addr);
            csr1 = val;
			if (csr1&CSR1_CPU_INT) {
				nd->nbic.set_intstatus(true);
			} else {
                nd->nbic.set_intstatus(false);
			}
            break;
        case 0x0020:
            Log_Printf(ND_LOG_IO_WR, MC_WR_FORMAT_S,"csr2", decodeBits(ND_CSR2_BITS, val),addr);
            csr2 = val;
            break;
        case 0x0030:
            Log_Printf(ND_LOG_IO_WR, MC_WR_FORMAT,"sid", val,addr);
            sid = val;
            break;
        case 0x1000:
            Log_Printf(ND_LOG_IO_WR, MC_WR_FORMAT_S,"dma_csr", decodeBits(ND_DMA_CSR_BITS, val),addr);
            dma_csr = val;
            break;
        case 0x1010:
            Log_Printf(ND_LOG_IO_WR, MC_WR_FORMAT,"dma_start", val,addr);
            dma_start = val;
            break;
        case 0x1020:
            Log_Printf(ND_LOG_IO_WR, MC_WR_FORMAT,"dma_width", val,addr);
            dma_width = val;
            break;
        case 0x1030:
            Log_Printf(ND_LOG_IO_WR, MC_WR_FORMAT,"dma_pstart", val,addr);
            dma_pstart = val;
            break;
        case 0x1040:
            Log_Printf(ND_LOG_IO_WR, MC_WR_FORMAT,"dma_pwidth", val,addr);
            dma_pwidth = val;
            break;
        case 0x1050:
            Log_Printf(ND_LOG_IO_WR, MC_WR_FORMAT,"dma_sstart", val,addr);
            dma_sstart = val;
            break;
        case 0x1060:
            Log_Printf(ND_LOG_IO_WR, MC_WR_FORMAT,"dma_swidth", val,addr);
            dma_swidth = val;
            break;
        case 0x1070:
            Log_Printf(ND_LOG_IO_WR, MC_WR_FORMAT,"dma_bsstart", val,addr);
            dma_bsstart = val;
            break;
        case 0x1080:
            Log_Printf(ND_LOG_IO_WR, MC_WR_FORMAT,"dma_bswidth", val,addr);
            dma_bswidth = val;
            break;
        case 0x1090:
            Log_Printf(ND_LOG_IO_WR, MC_WR_FORMAT,"dma_top", val,addr);
            dma_top = val;
            break;
        case 0x10A0:
            Log_Printf(ND_LOG_IO_WR, MC_WR_FORMAT,"dma_bottom", val,addr);
            dma_bottom = val;
            break;
        case 0x10B0:
            Log_Printf(ND_LOG_IO_WR, MC_WR_FORMAT,"dma_line_a", val,addr);
            dma_line_a = val;
            break;
        case 0x10C0:
            Log_Printf(ND_LOG_IO_WR, MC_WR_FORMAT,"dma_curr_a", val,addr);
            dma_curr_a = val;
            break;
        case 0x10D0:
            Log_Printf(ND_LOG_IO_WR, MC_WR_FORMAT,"dma_line_a", val,addr);
            dma_line_a = val;
            break;
        case 0x10E0:
            Log_Printf(ND_LOG_IO_WR, MC_WR_FORMAT,"dma_scurr_a", val,addr);
            dma_scurr_a = val;
            break;
        case 0x10F0:
            Log_Printf(ND_LOG_IO_WR, MC_WR_FORMAT,"dma_out_a", val,addr);
            dma_out_a = val;
            break;
        case 0x2000:
            Log_Printf(ND_LOG_IO_WR, MC_WR_FORMAT_S,"vram", decodeBits(ND_VRAM_BITS, val),addr);
            vram = val;
            break;
        case 0x3000:
            Log_Printf(ND_LOG_IO_WR, MC_WR_FORMAT_S,"dram", decodeBits(ND_DRAM_BITS, val),addr);
            dram = val;
            break;
        default:
            Log_Printf(LOG_WARN, "[ND] Memory controller UNKNOWN write at %08X",addr);
            break;
	}
}

/* NeXTdimension data path */

void DP::iicmsg(void) {
    Log_Printf(LOG_NONE, "[ND] data path IIC msg addr:%02X msg[%d]=%02X",
               iic_addr, iic_msgsz-1, iic_msg);
    
    nd->video_dev_write(iic_addr, iic_msgsz-1, iic_msg);
}

Uint32 DP::lget(Uint32 addr) {
    switch(addr) {
        case 0x300: case 0x304: case 0x308: case 0x30C:
        case 0x310: case 0x314: case 0x318: case 0x31C:
        case 0x320: case 0x324: case 0x328: case 0x32C:
        case 0x330: case 0x334: case 0x338: case 0x33C:
            return doff;
        case 0x340:
            return csr;
        case 0x344:
            return alpha;
        case 0x348:
            return dma;
        case 0x350:
            return cpu_x;
        case 0x354:
            return cpu_y;
        case 0x358:
            return dma_x;
        case 0x35C:
            return dma_y;
        case 0x360:
            if(iic_busy <= 0)
                iic_stat_addr &= ~DP_IIC_BUSY;
            else
                iic_busy--;
            return iic_stat_addr;
        case 0x364:
            return 0;
        default:
            Log_Printf(LOG_WARN, "[ND] data path UNKNOWN read at %08X",addr);
    }
    return 0;
}

void DP::lput(Uint32 addr, Uint32 v) {
    switch(addr) {
        case 0x300: case 0x304: case 0x308: case 0x30C:
        case 0x310: case 0x314: case 0x318: case 0x31C:
        case 0x320: case 0x324: case 0x328: case 0x32C:
        case 0x330: case 0x334: case 0x338: case 0x33C:
            doff  = v;
            break;
        case 0x340:
            csr = v;
            break;
        case 0x344:
            alpha = v;
            break;
        case 0x348:
            dma = v;
            break;
        case 0x350:
            cpu_x = v;
            break;
        case 0x354:
            cpu_y = v;
            break;
        case 0x358:
            dma_x = v;
            break;
        case 0x35C:
            dma_y = v;
            break;
        case 0x360:
            iic_msgsz = 0;
            iic_addr  = (v >> 8) & 0xFF;
            iic_msg   = v;
            iic_msgsz++;
            iic_stat_addr |= DP_IIC_BUSY;
            iic_busy       = 10;
            iicmsg();
            break;
        case 0x364:
            iic_msg = v;
            iic_msgsz++;
            iic_stat_addr |= DP_IIC_BUSY;
            iic_busy       = 10;
            iicmsg();
            break;
        default:
            Log_Printf(LOG_WARN, "[ND] data path UNKNOWN write at %08X %08X",addr,v);
    }
}

void NextDimension::set_blank_state(int src, bool state) {
    switch (src) {
        case ND_DISPLAY:
            if(state) {
                mc.csr0 |= CSR0_VBL_INT | CSR0_VBLANK;
                if (mc.csr0 & CSR0_VBL_IMASK) {
                    send_msg(MSG_INTR);
                }
            } else {
                mc.csr0 &= ~CSR0_VBLANK;
            }
            break;
        case ND_VIDEO:
            if(state) {
                mc.csr0 |= CSR0_VIOVBL_INT | CSR0_VIOBLANK;
                if (mc.csr0 & CSR0_VIOVBL_IMASK) {
                    send_msg(MSG_INTR);
                }
            } else {
                mc.csr0 &= ~CSR0_VIOBLANK;
            }
            break;
    }
}

static const char* nd_dump_path = "nd_memory.bin";

/* debugger stuff */
bool NextDimension::dbg_cmd(const char* buf) {
    if(!(buf)) {
        fprintf(stderr,
                "   w: write NeXTdimension DRAM to file '%s'\n"
                "   n: dump NeXTdimension registers\n"
                , nd_dump_path);
        return false;
    }
    
    switch(buf[0]) {
        case 'w': {
            FILE* fp = fopen(nd_dump_path, "wb");
            size_t size  = ConfigureParams.Dimension.board[ND_NUM(slot)].nMemoryBankSize[0];
            size        += ConfigureParams.Dimension.board[ND_NUM(slot)].nMemoryBankSize[1];
            size        += ConfigureParams.Dimension.board[ND_NUM(slot)].nMemoryBankSize[2];
            size        += ConfigureParams.Dimension.board[ND_NUM(slot)].nMemoryBankSize[3];
            fprintf(stderr, "Writing %" FMT_zu "MB to '%s'...", size, nd_dump_path);
            size <<= 20;
            fwrite(ram, sizeof(Uint8), size, fp);
            fclose(fp);
            fprintf(stderr, "done.");
            return true;
        }
        case 'n': {
            fprintf(stderr, "csr0        (%s)\n", decodeBits(ND_CSR0_BITS,    mc.csr0));
            fprintf(stderr, "csr1        (%s)\n", decodeBits(ND_CSR1_BITS,    mc.csr1));
            fprintf(stderr, "csr2        (%s)\n", decodeBits(ND_CSR2_BITS,    mc.csr2));
            fprintf(stderr, "sid         (%s)\n", decodeBits(0,               mc.sid));
            fprintf(stderr, "dma_csr     (%s)\n", decodeBits(ND_DMA_CSR_BITS, mc.dma_csr));
            fprintf(stderr, "dma_start   (%s)\n", decodeBits(0,               mc.dma_start));
            fprintf(stderr, "dma_width   (%s)\n", decodeBits(0,               mc.dma_width));
            fprintf(stderr, "dma_pstart  (%s)\n", decodeBits(0,               mc.dma_pstart));
            fprintf(stderr, "dma_pwidth  (%s)\n", decodeBits(0,               mc.dma_pwidth));
            fprintf(stderr, "dma_sstart  (%s)\n", decodeBits(0,               mc.dma_sstart));
            fprintf(stderr, "dma_swidth  (%s)\n", decodeBits(0,               mc.dma_swidth));
            fprintf(stderr, "dma_bsstart (%s)\n", decodeBits(0,               mc.dma_bsstart));
            fprintf(stderr, "dma_bswidth (%s)\n", decodeBits(0,               mc.dma_bswidth));
            fprintf(stderr, "dma_top     (%s)\n", decodeBits(0,               mc.dma_top));
            fprintf(stderr, "dma_bottom  (%s)\n", decodeBits(0,               mc.dma_bottom));
            fprintf(stderr, "dma_line_a  (%s)\n", decodeBits(0,               mc.dma_line_a));
            fprintf(stderr, "dma_curr_a  (%s)\n", decodeBits(0,               mc.dma_curr_a));
            fprintf(stderr, "dma_scurr_a (%s)\n", decodeBits(0,               mc.dma_scurr_a));
            fprintf(stderr, "dma_out_a   (%s)\n", decodeBits(0,               mc.dma_out_a));
            fprintf(stderr, "vram        (%s)\n", decodeBits(ND_VRAM_BITS,    mc.vram));
            fprintf(stderr, "dram        (%s)\n", decodeBits(ND_DRAM_BITS,    mc.dram));
            return true;
        }
        default:
            return false;
    }
}

