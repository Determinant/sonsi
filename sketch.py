class SyntaxObj(object):
    pass

class EvalObj(SyntaxObj):
    def __str__(self):
        return "#<Object>"

class ValObj(EvalObj):
    def __str__(self):
        return "#<Value>"

class NumberObj(ValObj):
    def __str__(selfl):
        return "#<Number>"

class IntObj(NumberObj):
    def __init__(self, num):
        self.val = int(num)
    def __str__(self):
        return "#<Integer>"

class FloatObj(NumberObj):
    def __init__(self, num):
        self.val = float(num)
    def __str__(self):
        return "#<Float>"

class StringObj(ValObj):
    def __init__(self, string):
        self.val = string

    def __str__(self):
        return "#<String>"

class OptObj(EvalObj):
    pass
class ProcObj(OptObj):
    def __init__(self, prog, env = None):
        self.prog = prog
        self.env = env

    def __str__(self):
        return "#<Procedure>"

class IdObj(SyntaxObj):
    def __init__(self, string):
        self.name = string
    def __str__(self):
        return "#<Identifier>"
    def get_name():
        return self.name

class Tokenizor():

    def __init__(self):
        self.data = ""
        self.tokenized = list()
        self.extended_chars = "!$%&*+-./:<=>?@^_~"

    def is_identifier(self, string):
        if string[0].isdigit(): return False
        for i in string[1:]:
            if not (i.isalnum() or i in self.extended_chars):
                return False
        return True

    def feed(self, data):
        self.data = data

    def read(self): 
        if len(self.tokenized) == 0:
            if len(self.data) == 0:
                return None
            self.tokenized = self.data.replace('(', '( ')\
                                    .replace(')', ' )')\
                                    .split()
            self.data = ""
        return self.tokenized.pop(0)

class Node(object):
    def __init__(self, syn_obj, sib = None, chd = None):
        self.obj = syn_obj
        self.sib = sib
        self.chd = chd
        self.typ = None # operator operand if-macro if-clause
    def __str__(self):
        return "#<AST Node>"
    def print_(self):
        print \
            "======================" + "\n" + \
            "Obj: " + str(self.obj) + "\n" + \
            "Sib: " + str(self.sib) + "\n" + \
            "Chd: " + str(self.chd) + "\n" + \
            "======================" 

class AbsSynTree(EvalObj):

    def to_node(self, obj):
        if isinstance(obj, Node): return obj
        # else the obj is a string
        try: return Node(IntObj(obj))
        except Exception:
            try: return Node(FloatObj(obj))
            except Exception: return Node(IdObj(obj))

    def __init__(self, stream):
        stack = list()
        while True:
            token = stream.read()
            if token is None: break
            if token == '(':
                stack.append(token) 
            elif token == ')':
                lst = list()
                while stack[-1] != '(':
                    lst = stack[-1:] + lst
                    del stack[-1]
                root = lst[0]
                if len(lst) > 1:
                    root.chd = lst[1]
                    ref = root.chd
                    for i in lst[2:]:
                        ref.sib = i
                        ref = ref.sib
                stack[-1] = root
            else:
                stack.append(self.to_node(token))
        self.tree = stack

def is_identifier(string):
    return isinstance(string, IdObj)
def is_val(node):
    return node.chd is None
def is_ret_addr(val):
    return isinstance(val, Node)

class Environment(object):
    def __init__(self):
        self.binding = dict()
    def add_binding(self, name, eval_obj):
        self.binding[name] = eval_obj
    def get_obj(self, id_obj):
        if is_identifier(id_obj):
            return self.binding[id_obj.name]
        else:
            return id_obj

class Continuation(object):
    def __init__(self, envt, pc, old_cont):    
        self.envt = envt
        self.pc = pc
        self.old_cont = old_cont
    def get_envt(self):
        return self.envt
    def get_cont(self):
        return self.cont

def _builtin_plus(arg_list):
    res = 0
    for i in arg_list:
        res += i.val
    return IntObj(res)

def _builtin_minus(arg_list):
    res = arg_list[0].val
    for i in arg_list[1:]:
        res -= i.val
    return IntObj(res)

def _builtin_times(arg_list):
    res = 1
    for i in arg_list:
        res *= i.val
    return IntObj(res)

def _builtin_div(arg_list):
    res = arg_list[0].val
    for i in arg_list[1:]:
        res /= i.val
    return IntObj(res)

_default_mapping = {
        "+" : _builtin_plus,
        "-" : _builtin_minus,
        "*" : _builtin_times,
        "/" : _builtin_div}

class Evaluator(object):

    def _add_builtin_routines(self, envt):
        for sym in _default_mapping:
            envt.add_binding(sym, _default_mapping[sym])

    def __init__(self):
        self.envt = Environment() # Top-level Env
        self._add_builtin_routines(self.envt)
    def run_expr(self, prog):
        stack = [0] * 100 # Stack 
        pc = prog.tree[0]  # Set to the root
        cont = None
        envt = self.envt
        top = -1 # Stack top

        def print_stack():
            print '++++++++++STACK++++++++'
            if len(stack) > 0:
                for i in range(0, top + 1):
                    print stack[i]
            print '----------STACK--------'
        
        def push(pc, top):
            ntop = top
            if is_val(pc):
                ntop += 1
                stack[ntop] = envt.get_obj(pc.obj)
                npc = pc.sib
                print "first"
                print "this val is: " + str(pc.obj)
            else:
                ntop += 1
                stack[ntop] = pc # Return address
                ntop += 1
                stack[ntop] = envt.get_obj(pc.obj)
                npc = pc.chd
                print "second"
            return (npc, ntop)

        print " Pushing..."
        print_stack()
        (pc, top) = push(pc, top)
        print_stack()
        print " Done...\n"
        while is_ret_addr(stack[0]):  # Still need to evaluate
            print "- Top: " + str(stack[top])
            print "- Pc at: " + str(pc)
            if pc is None:
                print "Poping..."
                arg_list = list()
                while not is_ret_addr(stack[top]):
                    arg_list = [stack[top]] + arg_list
                    top -= 1
                if is_identifier(arg_list[0]):
                    arg_list[0] = envt.get_obj(arg_list[0])
                pc = stack[top].sib
                stack[top] = arg_list[0](arg_list[1:])
                print_stack()
                print "RET to: "
                if pc:
                    pc.print_()
                    if is_identifier(pc.obj):
                        print pc.obj.name
                    else:
                        print pc.obj.val
                else: print "NULL"
            else: 
                print " Pushing..."
                print_stack()
                (pc, top) = push(pc, top)
                print_stack()
                print " Done...\n"
        return stack[0]


t = Tokenizor()
t.feed("(+ (- (* 10 2) (+ x 4)) (+ 1 2) (/ 25 5))")
#t.feed("((lambda (x) (x * x)) 2)")
a = AbsSynTree(t)
e = Evaluator()
e.envt.add_binding("x", IntObj(100))
print e.run_expr(a).val
