/*
 * Copyright 2014 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include "test-registry.hpp"

#include <jive/frontend/basic_block.h>
#include <jive/frontend/clg.h>
#include <jive/frontend/tac/tac.h>
#include <jive/types/bitstring/arithmetic.h>
#include <jive/types/bitstring/type.h>

#include <assert.h>

#define MAKE_OP_VERIFIER(NAME, OP) \
static void \
verify_##NAME##_op(jive::frontend::clg & clg) \
{ \
	jive::frontend::clg_node * node = clg.lookup_function("test_" #NAME); \
	assert(node != nullptr); \
\
	jive::frontend::cfg & cfg = node->cfg(); \
\
	assert(cfg.nnodes() == 3); \
	assert(cfg.is_linear()); \
\
	jive::frontend::basic_block * bb = dynamic_cast<jive::frontend::basic_block*>( \
		cfg.enter()->outedges()[0]->sink()); \
	assert(bb != nullptr); \
\
	std::vector<const jive::frontend::tac*> tacs = bb->tacs(); \
	assert(tacs.size() != 0); \
\
	jive::bits::type type(64); \
	jive::bits::OP op(type); \
	assert(tacs[0]->operation() == op); \
} \

MAKE_OP_VERIFIER(add, add_op);
MAKE_OP_VERIFIER(and, and_op);
MAKE_OP_VERIFIER(ashr, ashr_op);
MAKE_OP_VERIFIER(sub, sub_op);
MAKE_OP_VERIFIER(udiv, udiv_op);
MAKE_OP_VERIFIER(sdiv, sdiv_op);
MAKE_OP_VERIFIER(urem, umod_op);
MAKE_OP_VERIFIER(srem, smod_op);
MAKE_OP_VERIFIER(shl, shl_op);
MAKE_OP_VERIFIER(lshr, shr_op);
MAKE_OP_VERIFIER(or, or_op);
MAKE_OP_VERIFIER(xor, xor_op);
MAKE_OP_VERIFIER(mul, mul_op);

static int
verify(jive::frontend::clg & clg)
{
	verify_add_op(clg);
	verify_and_op(clg);
	verify_ashr_op(clg);
	verify_sub_op(clg);
	verify_udiv_op(clg);
	verify_sdiv_op(clg);
	verify_urem_op(clg);
	verify_srem_op(clg);
	verify_shl_op(clg);
	verify_lshr_op(clg);
	verify_or_op(clg);
	verify_xor_op(clg);
	verify_mul_op(clg);

	return 0;
}

JLM_UNIT_TEST_REGISTER("libjlm/instructions/test-bitarithmetic", verify);