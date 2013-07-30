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
class BoolObj(ValObj):
    def __init__(self, b):
        self.val = b
    def __str__(self):
        return "#<Boolean>"

class OptObj(EvalObj):
    pass

class ProcObj(OptObj):
    def __init__(self, prog, env = None):
        self.prog = prog
        self.env = env
    def __str__(self):
        return "#<Procedure>"

class SpecialOptObj(OptObj):
    def prepare(self, pc):
        pass
    def call(self, arg_list, pc, envt, cont):
        pass

class BuiltinProcObj():
    def __init__(self, f, ext_name):
        self.handler = f
        self.ext_name = ext_name
    def __str__(self):
        return self.ext_name
    def call(self, arg_list):
        return self.handler(arg_list)

def to_bool(obj):
    if obj.val == False:
        return BoolObj(False)
    else:
        return BoolObj(True)

class _builtin_if(SpecialOptObj):
    def prepare(self, pc):
        self.state = 0 # prepare
        # TODO: check number of arguments 
        return (True, False, False)
        # Delay the calculation
    def pre_call(self, arg_list, pc, envt, cont):
        self.state = 1 # calling
        print "Received if signal: " + str(arg_list[0].val)
        print "And it is regared as: " + str(to_bool(arg_list[0]).val)
        if (to_bool(arg_list[0])).val:
            return ((False, True, False), True) # Re-eval
        else:
            return ((False, False, True), True) # Re-eval
    def post_call(self, arg_list, pc, envt, cont):
        return (arg_list[0], False)

    def call(self, arg_list, pc, envt, cont):
        if self.state == 0:
            return self.pre_call(arg_list, pc, envt, cont)
        else:
            return self.post_call(arg_list, pc, envt, cont)
    def __str__(self):
        return "#<builtint opt if>"

class _builtin_lambda(SpecialOptObj):
    def prepare(self, pc):
        # TODO: check number of arguments 
        return [ False, False ]

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
        self.skip = None # delay calcuation
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
def is_leaf(node):
    return node.chd is None
def is_ret_addr(val):
    return isinstance(val, Node)
def is_builtin_proc(val):
    return isinstance(val, BuiltinProcObj)
def is_special_opt(val):
    return isinstance(val, SpecialOptObj)

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

def _builtin_lt(arg_list):
    #TODO: need support to multiple operands
    return BoolObj(arg_list[0].val < arg_list[1].val)

def _builtin_gt(arg_list):
    return BoolObj(arg_list[0].val > arg_list[1].val)

def _builtin_eq(arg_list):
    return BoolObj(arg_list[0].val == arg_list[1].val)

_default_mapping = {
        "+" : BuiltinProcObj(_builtin_plus, "builtin proc +"),
        "-" : BuiltinProcObj(_builtin_minus, "builtin proc -"),
        "*" : BuiltinProcObj(_builtin_times, "builtin proc *"),
        "/" : BuiltinProcObj(_builtin_div, "builtin proc /"),
        "<" : BuiltinProcObj(_builtin_lt, "builtin proc <"),
        ">" : BuiltinProcObj(_builtin_gt, "builtin proc >"),
        "=" : BuiltinProcObj(_builtin_eq, "builtin proc ="),
        "if" : _builtin_if()
        }

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
        
        def mask_eval(pc, mask):
            pc = pc.chd
            for i in mask:
                print i
                print pc
                pc.skip = not i
                pc = pc.sib

        def push(pc, top):
            ntop = top
            if is_leaf(pc):
                ntop += 1
                stack[ntop] = envt.get_obj(pc.obj)
                npc = pc.sib
                print "first"
                print "this val is: " + str(stack[ntop].val)
            else:
                ntop += 1
                stack[ntop] = pc # Return address
                ntop += 1
                stack[ntop] = envt.get_obj(pc.obj)
                npc = pc.chd
                if is_special_opt(stack[ntop]):
                    mask = stack[ntop].prepare(pc)
                    mask_eval(pc, mask)
                print "second"
            return (npc, ntop)

        print " Pushing..."
        print_stack()
        (pc, top) = push(pc, top)
        print_stack()
        print " Done...\n"
        while is_ret_addr(stack[0]):    # Still need to evaluate
            print "- Top: " + str(stack[top])
            print "- Pc at: " + str(pc)
            while pc and pc.skip: 
                print "skipping masked branch: " + str(pc)
                pc = pc.sib  # Skip the masked branches

            if pc is None:
                print "Poping..."
                arg_list = list()
                while not is_ret_addr(stack[top]):
                    arg_list = [stack[top]] + arg_list
                    top -= 1
                # Top is now pointing to the return address
                
                opt = arg_list[0]
                ret_addr = stack[top]
                if is_builtin_proc(opt):
                    stack[top] = opt.call(arg_list[1:])
                    pc = ret_addr.sib
                elif is_special_opt(opt):
                    (res, flag) = opt.call(arg_list[1:], pc, envt, cont)
                    if flag: # Need to call again
                        print "AGAIN with the mask: " + str(res)
                        mask_eval(ret_addr, res)
                        top += 1
                        pc = ret_addr.chd # Again
                    else:
                        stack[top] = res # Done
                        pc = ret_addr.sib

                print_stack()
            else: 
                print " Pushing..."
                print_stack()
                (pc, top) = push(pc, top)
                print_stack()
                print " Done...\n"
        return stack[0]


t = Tokenizor()
t.feed("(if (> 2 1) 0 1)")
#t.feed("(+ (- (* 10 2) (+ x 4)) (+ 1 2) (/ 25 5))")
#t.feed("((lambda (x) (x * x)) 2)")
a = AbsSynTree(t)
e = Evaluator()
e.envt.add_binding("x", IntObj(100))
print e.run_expr(a).val
