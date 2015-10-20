from llvmanalysis.graph import Graph, Node
from llvmsage.expr import Expr
from copy import copy

bottom_expr = Expr.get_minus_inf()
debug_flag = False

def debug(desc, *args):
  if debug_flag:
    print desc, ", ".join(map(str, args))

class Struct:
  def __init__(self, **entries):
    self.__dict__.update(entries)

  def __repr__(self):
    return repr(self.__dict__)

  def __str__(self):
    return str(self.__dict__)

class RaNode(Node):
  def __init__(self, name, helper, expr=bottom_expr, memid=0):
    Node.__init__(self, name, state=Struct(expr = expr, memid = memid))
    self.changed = Struct(lower = False, upper = False)
    self.args = []
    self.helper = helper

  def set_state(self, state):
    self.changed = self.state != state
    Node.set_state(self, state)

  def set_args(self, args):
    self.args = args

  def op_widen(self, state):
    assert False

class GeneratorNode(RaNode):
  def __init__(self, name, expr, memid, helper, **kwargs):
    RaNode.__init__(self, name, helper, expr = expr, memid = memid, **kwargs)

  def op(self, *nodes):
    debug("op (generator):", self)
    return self.state

class IdNode(RaNode):
  def __init__(self, name, helper, **kwargs):
    RaNode.__init__(self, name, helper, **kwargs)

  def op(self, state):
    debug("op (id_node):", self, state)
    return copy(state)

class ReplacerNode(RaNode):
  def __init__(self, name, repl, helper, **kwargs):
    RaNode.__init__(self, name, helper, **kwargs)
    self.repl = repl

  def op(self, incoming):
    debug("op (replace):", self, incoming)
    state = copy(incoming)

    if state.memid != 0:
      state.memid = copy(self.helper.get_next())

    for key, value in self.repl.iteritems():
      state.expr = state.expr.subs(key, value)
    return state

class IndexNode(RaNode):
  def __init__(self, name, helper, expr, **kwargs):
    RaNode.__init__(self, name, helper, **kwargs)
    self.expr = expr

  def op(self, incoming):
    debug("op (index):", self, incoming)
    state = copy(incoming)
    state.expr = state.expr - self.expr
    return state

class Helper:
  def __init__(self):
    self.number = 1

  def get_next(self):
    self.number = self.number + 1
    return self.number

class RAGraph(Graph):
  def __init__(self):
    Graph.__init__(self)
    self.helper = Helper()

  def get_generator(self, name, expr):
    node = GeneratorNode(name, expr, 0, self.helper)
    self.add_node(node)
    return node

  def get_noalias_generator(self, name, expr):
    node = GeneratorNode(name, expr, self.helper.get_next(), self.helper)
    self.add_node(node)
    return node

  def get_indexation(self, name, expr, size):
    node = IndexNode(name, expr * size, self.helper)
    self.add_node(node)
    return node

  def get_id(self, name):
    node = IdNode(name, self.helper)
    self.add_node(node)
    return node

  def get_replacer(self, name, repl):
    node = ReplacerNode(name, repl, self.helper)
    self.add_node(node)
    return node

