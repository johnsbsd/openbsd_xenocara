#include "ir3_nir.h"

#include "nir.h"
#include "nir_builder.h"
#include "nir_search.h"
#include "nir_search_helpers.h"

/* What follows is NIR algebraic transform code for the following 1
 * transforms:
 *    ('imul', 'a@32', 'b@32') => ('imadsh_mix16', 'b', 'a', ('imadsh_mix16', 'a', 'b', ('umul_low', 'a', 'b')))
 */

#ifndef NIR_OPT_ALGEBRAIC_STRUCT_DEFS
#define NIR_OPT_ALGEBRAIC_STRUCT_DEFS

struct transform {
   const nir_search_expression *search;
   const nir_search_value *replace;
   unsigned condition_offset;
};

struct per_op_table {
   const uint16_t *filter;
   unsigned num_filtered_states;
   const uint16_t *table;
};

/* Note: these must match the start states created in
 * TreeAutomaton._build_table()
 */

/* WILDCARD_STATE = 0 is set by zeroing the state array */
static const uint16_t CONST_STATE = 1;

#endif


   static const nir_search_variable search0_0 = {
   { nir_search_value_variable, 32 },
   0, /* a */
   false,
   nir_type_invalid,
   NULL,
   {0, 1, 2, 3},
};

static const nir_search_variable search0_1 = {
   { nir_search_value_variable, 32 },
   1, /* b */
   false,
   nir_type_invalid,
   NULL,
   {0, 1, 2, 3},
};
static const nir_search_expression search0 = {
   { nir_search_value_expression, 32 },
   false, false,
   0, 1,
   nir_op_imul,
   { &search0_0.value, &search0_1.value },
   NULL,
};

   /* replace0_0 -> search0_1 in the cache */

/* replace0_1 -> search0_0 in the cache */

/* replace0_2_0 -> search0_0 in the cache */

/* replace0_2_1 -> search0_1 in the cache */

/* replace0_2_2_0 -> search0_0 in the cache */

/* replace0_2_2_1 -> search0_1 in the cache */
static const nir_search_expression replace0_2_2 = {
   { nir_search_value_expression, 32 },
   false, false,
   0, 1,
   nir_op_umul_low,
   { &search0_0.value, &search0_1.value },
   NULL,
};
static const nir_search_expression replace0_2 = {
   { nir_search_value_expression, 32 },
   false, false,
   -1, 1,
   nir_op_imadsh_mix16,
   { &search0_0.value, &search0_1.value, &replace0_2_2.value },
   NULL,
};
static const nir_search_expression replace0 = {
   { nir_search_value_expression, 32 },
   false, false,
   -1, 1,
   nir_op_imadsh_mix16,
   { &search0_1.value, &search0_0.value, &replace0_2.value },
   NULL,
};


static const struct transform ir3_nir_lower_imul_state2_xforms[] = {
  { &search0, &replace0.value, 0 },
};

static const struct per_op_table ir3_nir_lower_imul_table[nir_num_search_ops] = {
   [nir_op_imul] = {
      .filter = (uint16_t []) {
         0,
         0,
         0,
      },
      
      .num_filtered_states = 1,
      .table = (uint16_t []) {
      
         2,
      },
   },
};

static void
ir3_nir_lower_imul_pre_block(nir_block *block, uint16_t *states)
{
   nir_foreach_instr(instr, block) {
      switch (instr->type) {
      case nir_instr_type_alu: {
         nir_alu_instr *alu = nir_instr_as_alu(instr);
         nir_op op = alu->op;
         uint16_t search_op = nir_search_op_for_nir_op(op);
         const struct per_op_table *tbl = &ir3_nir_lower_imul_table[search_op];
         if (tbl->num_filtered_states == 0)
            continue;

         /* Calculate the index into the transition table. Note the index
          * calculated must match the iteration order of Python's
          * itertools.product(), which was used to emit the transition
          * table.
          */
         uint16_t index = 0;
         for (unsigned i = 0; i < nir_op_infos[op].num_inputs; i++) {
            index *= tbl->num_filtered_states;
            index += tbl->filter[states[alu->src[i].src.ssa->index]];
         }
         states[alu->dest.dest.ssa.index] = tbl->table[index];
         break;
      }

      case nir_instr_type_load_const: {
         nir_load_const_instr *load_const = nir_instr_as_load_const(instr);
         states[load_const->def.index] = CONST_STATE;
         break;
      }

      default:
         break;
      }
   }
}

static bool
ir3_nir_lower_imul_block(nir_builder *build, nir_block *block,
                   const uint16_t *states, const bool *condition_flags)
{
   bool progress = false;

   nir_foreach_instr_reverse_safe(instr, block) {
      if (instr->type != nir_instr_type_alu)
         continue;

      nir_alu_instr *alu = nir_instr_as_alu(instr);
      if (!alu->dest.dest.is_ssa)
         continue;

      switch (states[alu->dest.dest.ssa.index]) {
      case 0:
         break;
      case 1:
         break;
      case 2:
         for (unsigned i = 0; i < ARRAY_SIZE(ir3_nir_lower_imul_state2_xforms); i++) {
            const struct transform *xform = &ir3_nir_lower_imul_state2_xforms[i];
            if (condition_flags[xform->condition_offset] &&
                nir_replace_instr(build, alu, xform->search, xform->replace)) {
               progress = true;
               break;
            }
         }
         break;
      default: assert(0);
      }
   }

   return progress;
}

static bool
ir3_nir_lower_imul_impl(nir_function_impl *impl, const bool *condition_flags)
{
   bool progress = false;

   nir_builder build;
   nir_builder_init(&build, impl);

   /* Note: it's important here that we're allocating a zeroed array, since
    * state 0 is the default state, which means we don't have to visit
    * anything other than constants and ALU instructions.
    */
   uint16_t *states = calloc(impl->ssa_alloc, sizeof(*states));

   nir_foreach_block(block, impl) {
      ir3_nir_lower_imul_pre_block(block, states);
   }

   nir_foreach_block_reverse(block, impl) {
      progress |= ir3_nir_lower_imul_block(&build, block, states, condition_flags);
   }

   free(states);

   if (progress) {
      nir_metadata_preserve(impl, nir_metadata_block_index |
                                  nir_metadata_dominance);
    } else {
#ifndef NDEBUG
      impl->valid_metadata &= ~nir_metadata_not_properly_reset;
#endif
    }

   return progress;
}


bool
ir3_nir_lower_imul(nir_shader *shader)
{
   bool progress = false;
   bool condition_flags[1];
   const nir_shader_compiler_options *options = shader->options;
   const shader_info *info = &shader->info;
   (void) options;
   (void) info;

   condition_flags[0] = true;

   nir_foreach_function(function, shader) {
      if (function->impl)
         progress |= ir3_nir_lower_imul_impl(function->impl, condition_flags);
   }

   return progress;
}

