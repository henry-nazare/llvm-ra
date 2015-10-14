from llvmanalysis.graph import Graph, Node
from llvmsage.expr import Expr

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

class RAGraph(Graph):
  def __init__(self):
    Graph.__init__(self)

  def get_generator(self, name, expr):
    node = GeneratorNode(name, expr)
    self.add_node(node)
    return node

