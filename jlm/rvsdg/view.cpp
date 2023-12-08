/*
 * Copyright 2010 2011 2012 Helge Bahmann <hcb@chaoticmind.net>
 * Copyright 2014 2015 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include <jlm/rvsdg/gamma.hpp>
#include <jlm/rvsdg/theta.hpp>
#include <jlm/rvsdg/view.hpp>
#include <jlm/rvsdg/traverser.hpp>
#include <jlm/rvsdg/type.hpp>

namespace jlm::rvsdg
{

static std::string
region_to_string(
    const jlm::rvsdg::region * region,
    size_t depth,
    std::unordered_map<const output *, std::string> &);

static std::string
indent(size_t depth)
{
  return std::string(depth * 2, ' ');
}

static std::string
create_port_name(
    const jlm::rvsdg::output * port,
    std::unordered_map<const output *, std::string> & map)
{
  std::string name = dynamic_cast<const jlm::rvsdg::argument *>(port) ? "a" : "o";
  name += jlm::util::strfmt(map.size());
  return name;
}

static std::string
node_to_string(
    const jlm::rvsdg::node * node,
    size_t depth,
    std::unordered_map<const output *, std::string> & map)
{
  std::string s(indent(depth));
  for (size_t n = 0; n < node->noutputs(); n++)
  {
    auto name = create_port_name(node->output(n), map);
    map[node->output(n)] = name;
    s = s + name + " ";
  }

  s += ":= " + node->operation().debug_string() + " ";

  for (size_t n = 0; n < node->ninputs(); n++)
  {
    s += map[node->input(n)->origin()];
    if (n <= node->ninputs() - 1)
      s += " ";
  }
  s += "\n";

  if (auto snode = dynamic_cast<const jlm::rvsdg::structural_node *>(node))
  {
    for (size_t n = 0; n < snode->nsubregions(); n++)
      s += region_to_string(snode->subregion(n), depth + 1, map);
  }

  return s;
}

static std::string
region_header(
    const jlm::rvsdg::region * region,
    std::unordered_map<const output *, std::string> & map)
{
  std::string header("[");
  for (size_t n = 0; n < region->narguments(); n++)
  {
    auto argument = region->argument(n);
    auto pname = create_port_name(argument, map);
    map[argument] = pname;

    header += pname;
    if (argument->input())
      header += jlm::util::strfmt(" <= ", map[argument->input()->origin()]);

    if (n < region->narguments() - 1)
      header += ", ";
  }
  header += "]{";

  return header;
}

static std::string
region_body(
    const jlm::rvsdg::region * region,
    size_t depth,
    std::unordered_map<const output *, std::string> & map)
{
  std::vector<std::vector<const jlm::rvsdg::node *>> context;
  for (const auto & node : region->nodes)
  {
    if (node.depth() >= context.size())
      context.resize(node.depth() + 1);
    context[node.depth()].push_back(&node);
  }

  std::string body;
  for (const auto & nodes : context)
  {
    for (const auto & node : nodes)
      body += node_to_string(node, depth, map);
  }

  return body;
}

static std::string
region_footer(
    const jlm::rvsdg::region * region,
    std::unordered_map<const output *, std::string> & map)
{
  std::string footer("}[");
  for (size_t n = 0; n < region->nresults(); n++)
  {
    auto result = region->result(n);
    auto pname = map[result->origin()];

    if (result->output())
      footer += map[result->output()] + " <= ";
    footer += pname;

    if (n < region->nresults() - 1)
      footer += ", ";
  }
  footer += "]";

  return footer;
}

static std::string
region_to_string(
    const jlm::rvsdg::region * region,
    size_t depth,
    std::unordered_map<const output *, std::string> & map)
{
  std::string s;
  s = indent(depth) + region_header(region, map) + "\n";
  s = s + region_body(region, depth + 1, map);
  s = s + indent(depth) + region_footer(region, map) + "\n";
  return s;
}

std::string
view(const jlm::rvsdg::region * region)
{
  std::unordered_map<const output *, std::string> map;
  return view(region, map);
}

std::string
view(const jlm::rvsdg::region * region, std::unordered_map<const output *, std::string> & map)
{
  return region_to_string(region, 0, map);
}

void
view(const jlm::rvsdg::region * region, FILE * out)
{
  fputs(view(region).c_str(), out);
  fflush(out);
}

std::string
region_tree(const jlm::rvsdg::region * region)
{
  std::function<std::string(const jlm::rvsdg::region *, size_t)> f =
      [&](const jlm::rvsdg::region * region, size_t depth)
  {
    std::string subtree;
    if (region->node())
    {
      if (region->node()->nsubregions() != 1)
      {
        subtree += std::string(depth, '-') + jlm::util::strfmt(region) + "\n";
        depth += 1;
      }
    }
    else
    {
      subtree = "ROOT\n";
      depth += 1;
    }

    for (const auto & node : region->nodes)
    {
      if (auto snode = dynamic_cast<const jlm::rvsdg::structural_node *>(&node))
      {
        subtree += std::string(depth, '-') + snode->operation().debug_string() + "\n";
        for (size_t n = 0; n < snode->nsubregions(); n++)
          subtree += f(snode->subregion(n), depth + 1);
      }
    }

    return subtree;
  };

  return f(region, 0);
}

void
region_tree(const jlm::rvsdg::region * region, FILE * out)
{
  fputs(region_tree(region).c_str(), out);
  fflush(out);
}

/* xml */

static inline std::string
xml_header()
{
  return "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
         "<rvsdg>\n";
}

static inline std::string
xml_footer()
{
  return "</rvsdg>\n";
}

static inline std::string
hex(size_t i){
  std::stringstream stream;
  stream << std::hex << i;
  return stream.str();
}

static inline std::string
id(const jlm::rvsdg::output * port)
{
  return jlm::util::strfmt("o", hex((intptr_t)port));
}

static inline std::string
id(const jlm::rvsdg::input * port)
{
  return jlm::util::strfmt("i", hex((intptr_t)port));
}

static inline std::string
id(const jlm::rvsdg::node * node)
{
  return jlm::util::strfmt("n", hex((intptr_t)node));
}

static inline std::string
id(const jlm::rvsdg::region * region)
{
  return jlm::util::strfmt("r", hex((intptr_t)region));
}

static inline std::string
argument_tag(const std::string & id)
{
  return "<argument id=\"" + id + "\"/>\n";
}

static inline std::string
result_tag(const std::string & id)
{
  return "<result id=\"" + id + "\"/>\n";
}

static inline std::string
input_tag(const std::string & id)
{
  return "<input id=\"" + id + "\"/>\n";
}

static inline std::string
output_tag(const std::string & id)
{
  return "<output id=\"" + id + "\"/>\n";
}

static inline std::string
node_starttag(const std::string & id, const std::string & name, const std::string & type)
{
  return "<node id=\"" + id + "\" name=\"" + name + "\" type=\"" + type + "\">\n";
}

static inline std::string
node_endtag()
{
  return "</node>\n";
}

static inline std::string
region_starttag(const std::string & id)
{
  return "<region id=\"" + id + "\">\n";
}

static inline std::string
region_endtag(const std::string & id)
{
  return "</region>\n";
}

static inline std::string
edge_tag(const std::string & srcid, const std::string & dstid)
{
  return "<edge source=\"" + srcid + "\" target=\"" + dstid + "\"/>\n";
}

static inline std::string
type(const jlm::rvsdg::node * n)
{
  if (dynamic_cast<const jlm::rvsdg::gamma_op *>(&n->operation()))
    return "gamma";

  if (dynamic_cast<const jlm::rvsdg::theta_op *>(&n->operation()))
    return "theta";

  return "";
}

static std::string
convert_region(const jlm::rvsdg::region * region);

static inline std::string
convert_simple_node(const jlm::rvsdg::simple_node * node)
{
  std::string s;

  s += node_starttag(id(node), node->operation().debug_string(), "");
  for (size_t n = 0; n < node->ninputs(); n++)
    s += input_tag(id(node->input(n)));
  for (size_t n = 0; n < node->noutputs(); n++)
    s += output_tag(id(node->output(n)));
  s += node_endtag();

  for (size_t n = 0; n < node->noutputs(); n++)
  {
    auto output = node->output(n);
    for (const auto & user : *output)
      s += edge_tag(id(output), id(user));
  }

  return s;
}

static inline std::string
convert_structural_node(const jlm::rvsdg::structural_node * node)
{
  std::string s;
  s += node_starttag(id(node), "", type(node));

  for (size_t n = 0; n < node->ninputs(); n++)
    s += input_tag(id(node->input(n)));
  for (size_t n = 0; n < node->noutputs(); n++)
    s += output_tag(id(node->output(n)));

  for (size_t n = 0; n < node->nsubregions(); n++)
    s += convert_region(node->subregion(n));
  s += node_endtag();

  for (size_t n = 0; n < node->noutputs(); n++)
  {
    auto output = node->output(n);
    for (const auto & user : *output)
      s += edge_tag(id(output), id(user));
  }

  return s;
}

static inline std::string
convert_node(const jlm::rvsdg::node * node)
{
  if (auto n = dynamic_cast<const simple_node *>(node))
    return convert_simple_node(n);

  if (auto n = dynamic_cast<const structural_node *>(node))
    return convert_structural_node(n);

  JLM_ASSERT(0);
  return "";
}

static inline std::string
convert_region(const jlm::rvsdg::region * region)
{
  std::string s;
  s += region_starttag(id(region));

  for (size_t n = 0; n < region->narguments(); n++)
    s += argument_tag(id(region->argument(n)));

  for (const auto & node : region->nodes)
    s += convert_node(&node);

  for (size_t n = 0; n < region->nresults(); n++)
    s += result_tag(id(region->result(n)));

  for (size_t n = 0; n < region->narguments(); n++)
  {
    auto argument = region->argument(n);
    for (const auto & user : *argument)
      s += edge_tag(id(argument), id(user));
  }

  s += region_endtag(id(region));

  return s;
}

std::string
to_xml(const jlm::rvsdg::region * region)
{
  std::string s;
  s += xml_header();

  s += convert_region(region);

  s += xml_footer();
  return s;
}

void
view_xml(const jlm::rvsdg::region * region, FILE * out)
{
  fputs(to_xml(region).c_str(), out);
  fflush(out);
}

std::string
get_dot_name(jlm::rvsdg::node *node){
  return jlm::util::strfmt("n",hex((intptr_t)node));
}

std::string
get_dot_name(jlm::rvsdg::output * output){
  if(dynamic_cast<jlm::rvsdg::argument*>(output)){
    return jlm::util::strfmt("a",hex((intptr_t)output),":","default");
  } else if(auto no = dynamic_cast<jlm::rvsdg::simple_output*>(output)){
    return jlm::util::strfmt(get_dot_name(no->node()),":","o",hex((intptr_t)output));
  } else if(dynamic_cast<jlm::rvsdg::structural_output*>(output)){
    return jlm::util::strfmt("so",hex((intptr_t)output),":","default");
  }
  JLM_UNREACHABLE("not implemented");
}

std::string
get_dot_name(jlm::rvsdg::input *input){
  if(dynamic_cast<jlm::rvsdg::result*>(input)){
    return jlm::util::strfmt("r",hex((intptr_t)input),":","default");
  } else if(auto ni = dynamic_cast<jlm::rvsdg::simple_input*>(input)){
    return jlm::util::strfmt(get_dot_name(ni->node()),":","i",hex((intptr_t)input));
  } else if(dynamic_cast<jlm::rvsdg::structural_input*>(input)){
    return jlm::util::strfmt("si",hex((intptr_t)input),":","default");
  }
  JLM_UNREACHABLE("not implemented");
}

std::string
port_to_dot(const std::string & display_name, const std::string & dot_name)
{
  auto dot =
      dot_name +
      " [shape=plaintext label=<\n"
      "            <TABLE BORDER=\"0\" CELLBORDER=\"0\" CELLSPACING=\"0\" CELLPADDING=\"0\">\n"
      "                <TR>\n"
      "                    <TD PORT=\"default\" BORDER=\"1\" CELLPADDING=\"1\"><FONT POINT-SIZE=\"10\">" + display_name
      +
      "</FONT></TD>\n"
      "                </TR>\n"
      "            </TABLE>\n"
      "> tooltip=\""+ dot_name+"\"];\n";
  return dot;
}

std::string
argument_to_dot(jlm::rvsdg::argument *argument) {
  auto display_name = jlm::util::strfmt("a",argument->index());
  auto dot_name = jlm::util::strfmt("a",hex((intptr_t)argument));
  return port_to_dot(display_name, dot_name);
}

std::string
result_to_dot(jlm::rvsdg::result * result) {
  auto display_name = jlm::util::strfmt("r", result->index());
  auto dot_name = jlm::util::strfmt("r",hex((intptr_t)result));
  return port_to_dot(display_name, dot_name);
}

std::string
structural_input_to_dot(jlm::rvsdg::structural_input * structuralInput) {
  auto display_name = jlm::util::strfmt("si", structuralInput->index());
  auto dot_name = jlm::util::strfmt("si",hex((intptr_t)structuralInput));
  return port_to_dot(display_name, dot_name);
}

std::string
structural_output_to_dot(jlm::rvsdg::structural_output * structuralOutput) {
  auto display_name = jlm::util::strfmt("so", structuralOutput->index());
  auto dot_name = jlm::util::strfmt("so",hex((intptr_t)structuralOutput));
  return port_to_dot(display_name, dot_name);
}

std::string
edge(jlm::rvsdg::output * output, jlm::rvsdg::input * input)
{
  auto color = "black";
  return get_dot_name(output) + " -> " + get_dot_name(input) + " [style=\"\", arrowhead=\"normal\", color=" + color +
         ", headlabel=<>, fontsize=10, labelangle=45, labeldistance=2.0, labelfontcolor=black];\n";
}

std::string
symbolic_edge(jlm::rvsdg::input * output, jlm::rvsdg::output * input)
{
  auto color = "black";
  return get_dot_name(output) + " -> " + get_dot_name(input) + " [style=\"\", arrowhead=\"normal\", color=" + color +
         ", headlabel=<>, fontsize=10, labelangle=45, labeldistance=2.0, labelfontcolor=black];\n";
}

bool
isForbiddenChar(char c) {
  if (('A' <= c && c <= 'Z') ||
      ('a' <= c && c <= 'z') ||
      ('0' <= c && c <= '9') ||
      '_' == c) {
    return false;
  }
  return true;
}

std::string
structural_node_to_dot(jlm::rvsdg::structural_node * structuralNode){

  std::ostringstream dot;
  dot << "subgraph cluster_sn" << hex((intptr_t)structuralNode) << " {\n";
  dot << "color=\"#ff8080\"\n";
  dot << "penwidth=6\n";

  // input nodes
  for (size_t i = 0; i < structuralNode->ninputs(); ++i) {
    dot << structural_input_to_dot(structuralNode->input(i));
  }

  if(structuralNode->ninputs()>1){

    // order inputs horizontally
    dot << "{rank=source; ";
    for (size_t i = 0; i < structuralNode->ninputs(); ++i) {
      if(i>0){
        dot << " -> ";
      }
      dot << get_dot_name(structuralNode->input(i));
    }
    dot << "[style = invis]}\n";
  }

  for (size_t i = 0; i < structuralNode->nsubregions(); ++i)
  {
    dot << region_to_dot(structuralNode->subregion(i));
  }

  for (size_t i = 0; i < structuralNode->ninputs(); ++i) {
    for (auto &argument : structuralNode->input(i)->arguments){
      dot << symbolic_edge(structuralNode->input(i), &argument);
    }
  }

  // output nodes
  for (size_t i = 0; i < structuralNode->noutputs(); ++i) {
    dot << structural_output_to_dot(structuralNode->output(i));
    for (auto &result : structuralNode->output(i)->results){
      dot << symbolic_edge(&result, structuralNode->output(i));
    }
  }
  if(structuralNode->noutputs()>1) {
    // order results horizontally
    dot << "{rank=sink; ";
    for (size_t i = 0; i < structuralNode->noutputs(); ++i) {
      if (i > 0)
      {
        dot << " -> ";
      }
      dot << get_dot_name(structuralNode->output(i));
    }
    dot << "[style = invis]}\n";
  }

  dot << "}\n";

  return dot.str();
}

std::string
simple_node_to_dot(jlm::rvsdg::simple_node *simpleNode){
  auto SPACER = "                    <TD WIDTH=\"10\"></TD>\n";
  auto name = get_dot_name(simpleNode);
  auto opname = simpleNode->operation().debug_string();
  std::replace_if(opname.begin(), opname.end(), isForbiddenChar, '_');

  std::ostringstream inputs;
  // inputs
  for (size_t i = 0; i < simpleNode->ninputs(); ++i) {
    if (i != 0)
    {
      inputs << SPACER;
    }
    inputs << "                    <TD PORT=\"i" << hex((intptr_t)simpleNode->input(i))
        << "\" BORDER=\"1\" CELLPADDING=\"1\"><FONT POINT-SIZE=\"10\"> i" << i << "</FONT></TD>\n";
  }

  std::ostringstream outputs;
  // inputs
  for (size_t i = 0; i < simpleNode->noutputs(); ++i) {
    if (i != 0)
    {
      outputs << SPACER;
    }
    outputs << "                    <TD PORT=\"o" << hex((intptr_t)simpleNode->output(i))
        << "\" BORDER=\"1\" CELLPADDING=\"1\"><FONT POINT-SIZE=\"10\"> o" << i << "</FONT></TD>\n";
  }

  std::string color = "black";
  auto dot = \
      name +
      " [shape=plaintext label=<\n"
      "<TABLE BORDER=\"0\" CELLBORDER=\"0\" CELLSPACING=\"0\" CELLPADDING=\"0\">\n"
      // inputs
      "    <TR>\n"
      "        <TD BORDER=\"0\">\n"
      "            <TABLE BORDER=\"0\" CELLBORDER=\"0\" CELLSPACING=\"0\" CELLPADDING=\"0\">\n"
      "                <TR>\n"
      "                    <TD WIDTH=\"20\"></TD>\n" +
      inputs.str() +
      "                    <TD WIDTH=\"20\"></TD>\n"
      "                </TR>\n"
      "            </TABLE>\n"
      "        </TD>\n"
      "    </TR>\n"
      "    <TR>\n"
      "        <TD BORDER=\"3\" STYLE=\"ROUNDED\" CELLPADDING=\"4\">" +
      opname +
      "<BR/><FONT POINT-SIZE=\"10\">" +
      name +
      "</FONT></TD>\n"
      "    </TR>\n"
      "    <TR>\n"
      "        <TD BORDER=\"0\">\n"
      "            <TABLE BORDER=\"0\" CELLBORDER=\"0\" CELLSPACING=\"0\" CELLPADDING=\"0\">\n"
      "                <TR>\n"
      "                    <TD WIDTH=\"20\"></TD>\n" +
      outputs.str() +
      "                    <TD WIDTH=\"20\"></TD>\n"
      "                </TR>\n"
      "            </TABLE>\n"
      "        </TD>\n"
      "    </TR>\n"
      "</TABLE>\n"
      "> fontcolor=" +
      color + " color=" + color + "];\n";
  return dot;
}

std::string
region_to_dot(jlm::rvsdg::region * region){
  std::ostringstream dot;
  dot << "subgraph cluster_reg" << hex((intptr_t)region) << " {\n";
  dot << "color=\"#80b3ff\"\n";
  dot << "penwidth=6\n";

  // argument nodes
  for (size_t i = 0; i < region->narguments(); ++i) {
    dot << argument_to_dot(region->argument(i));
  }

  if(region->narguments()>1){
    // order arguments horizontally
    dot << "{rank=source; ";
    for (size_t i = 0; i < region->narguments(); ++i) {
      if(i>0){
        dot << " -> ";
      }
      dot << get_dot_name(region->argument(i));
    }
    dot << "[style = invis]}\n";
  }

  // nodes
  for (auto node: jlm::rvsdg::topdown_traverser(region))
  {
    if (auto simpleNode = dynamic_cast<jlm::rvsdg::simple_node *>(node)) {
      auto node_dot = simple_node_to_dot(simpleNode);
      dot << node_dot;
    } else if(auto structuralNode = dynamic_cast<jlm::rvsdg::structural_node*>(node)) {
      auto node_dot = structural_node_to_dot(structuralNode);
      dot << node_dot;
    }

    for (size_t i = 0; i < node->ninputs(); ++i) {
      dot << edge(node->input(i)->origin(), node->input(i));
    }
  }

  // result nodes
  for (size_t i = 0; i < region->nresults(); ++i) {
    dot << result_to_dot(region->result(i));
    dot << edge(region->result(i)->origin(), region->result(i));
  }

  if(region->nresults()>1) {
    // order results horizontally
    dot << "{rank=sink; ";
    for (size_t i = 0; i < region->nresults(); ++i)
    {
      if (i > 0)
      {
        dot << " -> ";
      }
      dot << get_dot_name(region->result(i));
    }
    dot << "[style = invis]}\n";
  }

  dot << "}\n";

  return dot.str();
}

std::string
to_dot(jlm::rvsdg::region * region)
{
  std::ostringstream dot;
  dot << "digraph G {\n";
  dot << region_to_dot(region);
  dot << "}\n";
  return dot.str();
}

void
view_dot(jlm::rvsdg::region * region, FILE * out)
{
  fputs(to_dot(region).c_str(), out);
  fflush(out);
}

}
