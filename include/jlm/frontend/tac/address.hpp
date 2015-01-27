/*
 * Copyright 2014 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#ifndef JLM_FRONTEND_TAC_ADDRESS_H
#define JLM_FRONTEND_TAC_ADDRESS_H

#include <jlm/frontend/tac/tac.hpp>

#include <jive/arch/address.h>
#include <jive/arch/addresstype.h>
#include <jive/arch/load.h>
#include <jive/arch/memorytype.h>
#include <jive/arch/store.h>
#include <jive/types/bitstring/type.h>

namespace jlm {
namespace frontend {

JIVE_EXPORTED_INLINE const jlm::frontend::output *
addrload_tac(jlm::frontend::basic_block * basic_block, const jlm::frontend::output * address,
	const jlm::frontend::output * state, const jive::value::type & data_type)
{
	jive::addr::type addrtype;
	std::vector<std::unique_ptr<jive::state::type>> state_type;
	state_type.emplace_back(std::unique_ptr<jive::state::type>(new jive::mem::type()));
	jive::load_op op(addrtype, state_type, data_type);
	const jlm::frontend::tac * tac = basic_block->append(op, {address, state});
	return tac->outputs()[0];
}

JIVE_EXPORTED_INLINE const jlm::frontend::output *
addrstore_tac(jlm::frontend::basic_block * basic_block, const jlm::frontend::output * address,
	const jlm::frontend::output * value, const jlm::frontend::output * state)
{
	jive::addr::type addrtype;
	std::vector<std::unique_ptr<jive::state::type>> state_type;
	state_type.emplace_back(std::unique_ptr<jive::state::type>(new jive::mem::type()));
	jive::store_op op(addrtype, state_type, dynamic_cast<const jive::value::type&>(value->type()));
	const jlm::frontend::tac * tac = basic_block->append(op, {address, value, state});
	return tac->outputs()[0];
}

JIVE_EXPORTED_INLINE const jlm::frontend::output *
addrarraysubscript_tac(jlm::frontend::basic_block * basic_block,
	const jlm::frontend::output * base, const jlm::frontend::output * offset)
{
	const jive::value::type & base_type = dynamic_cast<const jive::value::type&>(base->type());
	const jive::bits::type & offset_type = dynamic_cast<const jive::bits::type&>(offset->type());
	jive::address::arraysubscript_op op(base_type, offset_type);
	const jlm::frontend::tac * tac = basic_block->append(op, {base, offset});
	return tac->outputs()[0];
}

}
}

#endif
