/*
 * Copyright 2017 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include "test-registry.hpp"

#include <jive/view.h>
#include <jive/vsdg/graph.h>

#include <jlm/ir/module.hpp>
#include <jlm/jlm2llvm/jlm2llvm.hpp>
#include <jlm/jlm2rvsdg/module.hpp>
#include <jlm/llvm2jlm/module.hpp>
#include <jlm/rvsdg2jlm/rvsdg2jlm.hpp>

#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>

static int
test()
{
	using namespace llvm;

	LLVMContext ctx;
	std::unique_ptr<Module> module(new llvm::Module("module", ctx));

	auto i32 = Type::getInt32Ty(ctx);
	auto el = GlobalValue::ExternalLinkage;

	new GlobalVariable(*module, i32, true, el, ConstantInt::get(i32, 42), "gv1");

	module->dump();

	using namespace jlm;

	auto m = convert_module(*module);
	auto rvsdg = construct_rvsdg(*m);
	jive::view(rvsdg->root(), stdout);

	m = rvsdg2jlm::rvsdg2jlm(*rvsdg);
	module = jlm2llvm::convert(*m, ctx);
	module->dump();

	return 0;
}

JLM_UNIT_TEST_REGISTER("libjlm/test-global_variables", test);