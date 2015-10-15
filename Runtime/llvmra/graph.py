from llvmanalysis.graph import Graph, Node
from llvmsage.expr import Expr

import operator

bottom_expr = Expr.get_minus_inf()
debug_flag = False

def debug(desc, *args):
  if debug_flag:
    print desc, ", ".join(args)

class Struct:
  def __init__(self, **entries):
    self.__dict__.update(entries)

class RaNode(Node):
  def __init__(self, name, state=bottom_expr):
    Node.__init__(self, name, state=state)
    self.changed = Struct(lower = False, upper = False)
    self.args = []

  def set_state(self, state):
    self.changed = self.state != state
    Node.set_state(self, state)

  def set_args(self, args):
    self.args = args

  def op_widen(self, state):
    assert False

class GeneratorNode(RaNode):
  def __init__(self, name, expr, **kwargs):
    RaNode.__init__(self, name, **kwargs)
    self.expr = expr

  def op(self, *nodes):
    debug("op (generator):", self)
    return self.expr

class ReplacerNode(RaNode):
  def __init__(self, name, repl, **kwargs):
    RaNode.__init__(self, name, **kwargs)
    self.is_instanced = True
    self.repl = repl

  def op(self, incoming):
    debug("op (replace):", self, incoming)
    print "repl:", self.repl
    for key, value in self.repl.iteritems():
      state = state.subs(key, value)
    return state

class IndexNode(RaNode):
  def __init__(self, name, expr, **kwargs):
    RaNode.__init__(self, name, **kwargs)
    self.expr = expr

  def op(self, incoming):
    debug("op (index):", self, incoming)
    return incoming - self.expr

class RAGraph(Graph):
  def __init__(self):
    Graph.__init__(self)

  def get_generator(self, name, expr):
    node = GeneratorNode(name, expr)
    self.add_node(node)
    return node

  def get_indexation(self, name, expr, size):
    node = IndexNode(name, expr * size)
    self.add_node(node)
    return node

  def get_id(self, name):
    node = PhiNode(name)
    self.add_node(node)
    return node

  def get_replacer(self, name, repl):
    node = ReplacerNode(name, repl)
    self.add_node(node)
    return node

