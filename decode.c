#include "decode.h"
#include "util.h"

static struct intr *decode_arm(struct mm *mm, struct xlat *xlat, gaddr_t gaddr, struct hargs *hargs) {
    struct intr *intr = calloc(1, sizeof(struct intr));
    if (!intr) fatal("Out of memory");

    intr->insn_sz = 4;

    uint32_t *insn = (uint32_t *)(mm->base + gaddr);
    if (gaddr + intr->insn_sz > mm->size) fatal("Address 0x%08x out of guest memory range", gaddr);

    switch (*insn & ARM_FAMILY_MASK) {
    case ARM_FAMILY_DATA_OPS_REG:
        if ((*insn & ARM_DATA_OPS_REG_MULT_BITS) == ARM_DATA_OPS_REG_MULT_BITS) {
            intr->operation = INTR_OPERATION_MULTIPLY;
            intr->opcode =    (*insn >> ARM_DATA_OPS_REG_MULT_OPCODE_SHFT) & ARM_DATA_OPS_REG_MULT_OPCODE_MASK;
            intr->set_flags = (*insn >> ARM_DATA_OPS_REG_MULT_SET_FLAGS_SHFT) & ARM_DATA_OPS_REG_MULT_SET_FLAGS_MASK;
            intr->dest_reg =  (*insn >> ARM_DATA_OPS_REG_MULT_DEST_REG_SHFT) & ARM_DATA_OPS_REG_MULT_DEST_REG_MASK;
            intr->accm_reg =  (*insn >> ARM_DATA_OPS_REG_MULT_ACCM_REG_SHFT) & ARM_DATA_OPS_REG_MULT_ACCM_REG_MASK;
            intr->shft_reg =  (*insn >> ARM_DATA_OPS_REG_SHFT_REG_SHFT) & ARM_DATA_OPS_REG_MULT_SHFT_REG_MASK;
            intr->srcm_reg =  (*insn >> ARM_DATA_OPS_REG_MULT_SRCM_REG_SHFT) & ARM_DATA_OPS_REG_MULT_SRCM_REG_MASK;
        } else if ((*insn & ARM_DATA_OPS_REG_MISC_BITS) == ARM_DATA_OPS_REG_MISC_BITS) {
            // TODO: miscellaneous
        } else {
            intr->operation = INTR_OPERATION_DATA_OPS;
            intr->opcode =    (*insn >> ARM_DATA_OPS_REG_OPCODE_SHFT) & ARM_DATA_OPS_REG_OPCODE_MASK;
            intr->set_flags = (*insn >> ARM_DATA_OPS_REG_SET_FLAGS_SHFT) & ARM_DATA_OPS_REG_SET_FLAGS_MASK;
            intr->srcn_reg =  (*insn >> ARM_DATA_OPS_REG_SRCN_REG_SHFT) & ARM_DATA_OPS_REG_SRCN_REG_MASK;
            intr->dest_reg =  (*insn >> ARM_DATA_OPS_REG_DEST_REG_SHFT) & ARM_DATA_OPS_REG_DEST_REG_MASK;
            intr->shft_reg =  (*insn >> ARM_DATA_OPS_REG_SHFT_REG_SHFT) & ARM_DATA_OPS_REG_SHFT_REG_MASK;
            intr->shft_imm =  (*insn >> ARM_DATA_OPS_REG_SHFT_IMM_SHFT) & ARM_DATA_OPS_REG_SHFT_IMM_MASK;
            intr->shft_type = (*insn >> ARM_DATA_OPS_REG_SHFT_TYPE_SHFT) & ARM_DATA_OPS_REG_SHFT_TYPE_MASK;
            intr->shft_src =  (*insn >> ARM_DATA_OPS_REG_SHFT_SRC_SHFT) & ARM_DATA_OPS_REG_SHFT_SRC_MASK;
            intr->srcm_reg =  (*insn >> ARM_DATA_OPS_REG_SRCM_REG_SHFT) & ARM_DATA_OPS_REG_SRCM_REG_MASK;
            if (intr->shft_src == ARM_DATA_OPS_REG_SHFT_SRC_IMM && intr->shft_imm == 0) {
                switch (intr->shft_type) {
                case ARM_DATA_OPS_REG_SHFT_TYPE_LSL:
                    break;
                case ARM_DATA_OPS_REG_SHFT_TYPE_LSR:
                case ARM_DATA_OPS_REG_SHFT_TYPE_ASR:
                    intr->shft_imm = 32;
                    break;
                case ARM_DATA_OPS_REG_SHFT_TYPE_ROR:
                    intr->shft_type = ARM_DATA_OPS_REG_SHFT_TYPE_RRX;
                    intr->shft_imm = 1;
                    break;
                default:
                    fatal("Logic error: control reached default case of arm data ops (reg) shift type switch");
                }
            }
            if (intr->dest_reg == CPU_REG_PC && (intr->opcode < 8 || intr->opcode > 11)) intr->flags |= INTR_FLAGS_WRITE_PC;
        }
        break;
    case ARM_FAMILY_DATA_OPS_IMM:
    case ARM_FAMILY_DATA_MOV_IMM:
    case ARM_FAMILY_DATA_MOV_REG:
    case ARM_FAMILY_DATA_MOV_MUL:
    case ARM_FAMILY_BRANCH:
    case ARM_FAMILY_DATA_MOV_COP:
    case ARM_FAMILY_OTHER:
        intr->flags |= INTR_FLAGS_TODO;
        break;
    default:
        fatal("Logic error: control reached default case of arm family switch");
    }

    if (hargs->flags & HARGS_DEBUG) {
        debug("Decoded instruction at 0x%08x:", gaddr);
        debug("  insn_sz = %u", intr->insn_sz);
        debug("  flags = 0x%08x", intr->flags);
        debug("  condition = %u", intr->condition);
        debug("  operation = %u", intr->operation);
        debug("  opcode = %u", intr->opcode);
        debug("  set_flags = %u", intr->set_flags);
        debug("  dest_reg = %u", intr->dest_reg);
        debug("  srcm_reg = %u", intr->srcm_reg);
        debug("  srcn_reg = %u", intr->srcn_reg);
        debug("  accm_reg = %u", intr->accm_reg);
        debug("  shft_reg = %u", intr->shft_reg);
        debug("  shft_imm = %u", intr->shft_imm);
        debug("  shft_src = %u", intr->shft_src);
        debug("  shft_type = %u", intr->shft_type);
    }

    return intr;
}

static struct intr *decode_thumb(struct mm *mm, struct xlat *xlat, gaddr_t gaddr, struct hargs *hargs) {
    struct intr *intr = calloc(1, sizeof(struct intr));
    if (!intr) fatal("Out of memory");

    intr->flags |= INTR_FLAGS_TODO;

    return intr;
}

struct intr *decode(struct mm *mm, struct xlat *xlat, gmeta_t gmeta, struct hargs *hargs) {
    return ga_thumb(gmeta) ? decode_thumb(mm, xlat, gaddr_of(gmeta), hargs) : decode_arm(mm, xlat, gaddr_of(gmeta), hargs);
}
