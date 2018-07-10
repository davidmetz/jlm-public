/*
 * Copyright 2015 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include "test-registry.hpp"

#include <jlm/jlm/ir/basic-block.hpp>
#include <jlm/jlm/ir/cfg.hpp>
#include <jlm/jlm/ir/cfg-structure.hpp>
#include <jlm/jlm/ir/module.hpp>
#include <jlm/jlm/ir/print.hpp>
#include <jlm/jlm/jlm2rvsdg/restructuring.hpp>

#include <assert.h>

static inline void
test_acyclic_structured()
{
	using namespace jlm;

	jlm::module module("", "");

	jlm::cfg cfg(module);
	auto bb1 = basic_block::create(cfg);
	auto bb2 = basic_block::create(cfg);
	auto bb3 = basic_block::create(cfg);
	auto bb4 = basic_block::create(cfg);

	cfg.exit_node()->divert_inedges(bb1);
	bb1->add_outedge(bb2);
	bb1->add_outedge(bb3);
	bb2->add_outedge(bb4);
	bb3->add_outedge(bb4);
	bb4->add_outedge(cfg.exit_node());

//	jlm::view_ascii(cfg, stdout);

	size_t nnodes = cfg.nnodes();
	restructure_branches(&cfg);

//	jlm::view_ascii(cfg, stdout);

	assert(nnodes == cfg.nnodes());
}

static inline void
test_acyclic_unstructured()
{
	using namespace jlm;

	jlm::module module("", "");

	jlm::cfg cfg(module);
	auto bb1 = basic_block::create(cfg);
	auto bb2 = basic_block::create(cfg);
	auto bb3 = basic_block::create(cfg);
	auto bb4 = basic_block::create(cfg);

	cfg.exit_node()->divert_inedges(bb1);
	bb1->add_outedge(bb2);
	bb1->add_outedge(bb3);
	bb2->add_outedge(bb3);
	bb2->add_outedge(bb4);
	bb3->add_outedge(bb4);
	bb4->add_outedge(cfg.exit_node());

//	jlm::view_ascii(cfg, stdout);

	restructure_branches(&cfg);

//	jlm::view_ascii(cfg, stdout);

	assert(is_proper_structured(cfg));
}

static inline void
test_dowhile()
{
	using namespace jlm;

	jlm::module module("", "");

	jlm::cfg cfg(module);
	auto bb1 = basic_block::create(cfg);
	auto bb2 = basic_block::create(cfg);
	auto bb3 = basic_block::create(cfg);

	cfg.exit_node()->divert_inedges(bb1);
	bb1->add_outedge(bb2);
	bb2->add_outedge(bb2);
	bb2->add_outedge(bb3);
	bb3->add_outedge(bb1);
	bb3->add_outedge(cfg.exit_node());

//	jlm::view_ascii(cfg, stdout);

	size_t nnodes = cfg.nnodes();
	restructure(&cfg);

//	jlm::view_ascii(cfg, stdout);

	assert(nnodes == cfg.nnodes());
	assert(bb2->outedge(0)->sink() == bb2);
	assert(bb3->outedge(0)->sink() == bb1);
}

static inline void
test_while()
{
	using namespace jlm;

	jlm::module module("", "");

	jlm::cfg cfg(module);
	auto bb1 = basic_block::create(cfg);
	auto bb2 = basic_block::create(cfg);

	cfg.exit_node()->divert_inedges(bb1);
	bb1->add_outedge(cfg.exit_node());
	bb1->add_outedge(bb2);
	bb2->add_outedge(bb1);

//	jlm::view_ascii(cfg, stdout);

	restructure(&cfg);

	/* FIXME: Nodes are not printed in the right order */
//	jlm::view_ascii(cfg, stdout);

	assert(is_proper_structured(cfg));
}

static inline void
test_irreducible()
{
	using namespace jlm;

	jlm::module module("", "");

	jlm::cfg cfg(module);
	auto bb1 = basic_block::create(cfg);
	auto bb2 = basic_block::create(cfg);
	auto bb3 = basic_block::create(cfg);
	auto bb4 = basic_block::create(cfg);
	auto bb5 = basic_block::create(cfg);

	cfg.exit_node()->divert_inedges(bb1);
	bb1->add_outedge(bb2);
	bb1->add_outedge(bb3);
	bb2->add_outedge(bb4);
	bb2->add_outedge(bb3);
	bb3->add_outedge(bb2);
	bb3->add_outedge(bb5);
	bb4->add_outedge(cfg.exit_node());
	bb5->add_outedge(cfg.exit_node());

//	jlm::view_ascii(cfg, stdout);

	restructure(&cfg);

//	jlm::view_ascii(cfg, stdout);
	assert(is_proper_structured(cfg));
}

static inline void
test_acyclic_unstructured_in_dowhile()
{
	using namespace jlm;

	jlm::module module("", "");

	jlm::cfg cfg(module);
	auto bb1 = basic_block::create(cfg);
	auto bb2 = basic_block::create(cfg);
	auto bb3 = basic_block::create(cfg);
	auto bb4 = basic_block::create(cfg);

	cfg.exit_node()->divert_inedges(bb1);
	bb1->add_outedge(bb3);
	bb1->add_outedge(bb2);
	bb2->add_outedge(bb3);
	bb2->add_outedge(bb4);
	bb3->add_outedge(bb4);
	bb4->add_outedge(bb1);
	bb4->add_outedge(cfg.exit_node());

//	jlm::view_ascii(cfg, stdout);

	restructure(&cfg);

//	jlm::view_ascii(cfg, stdout);
	assert(is_proper_structured(cfg));
}

static inline void
test_lor_before_dowhile()
{
	using namespace jlm;

	jlm::module module("", "");

	jlm::cfg cfg(module);
	auto bb1 = basic_block::create(cfg);
	auto bb2 = basic_block::create(cfg);
	auto bb3 = basic_block::create(cfg);
	auto bb4 = basic_block::create(cfg);

	cfg.exit_node()->divert_inedges(bb1);
	bb1->add_outedge(bb2);
	bb1->add_outedge(bb3);
	bb2->add_outedge(bb4);
	bb2->add_outedge(bb3);
	bb3->add_outedge(bb4);
	bb4->add_outedge(cfg.exit_node());
	bb4->add_outedge(bb4);

//	jlm::view_ascii(cfg, stdout);

	restructure(&cfg);

//	jlm::view_ascii(cfg, stdout);
	assert(is_proper_structured(cfg));
}

static int
verify()
{
	test_acyclic_structured();
	test_acyclic_unstructured();
	test_dowhile();
	test_while();
	test_irreducible();
	test_acyclic_unstructured_in_dowhile();
	test_lor_before_dowhile();

	return 0;
}

JLM_UNIT_TEST_REGISTER("libjlm/test-restructuring", verify);
