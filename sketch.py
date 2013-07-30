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
    def __init__(self, body, envt, para_list):
        self.body = body
        self.envt = envt
        self.para_list = para_list
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
        return "#<builtin opt if>"

class _builtin_lambda(SpecialOptObj):
    def prepare(self, pc):
        # TODO: check number of arguments 
        return (False, False)
    def call(self, arg_list, pc, envt, cont):
        para_list = list()
        par = pc.chd
        if par.obj:
            para_list.append(par.obj)
            if par.chd: 
                par = par.chd
                while par: 
                    para_list.append(par.obj)
                    par = par.sib
        body = pc.chd.sib
        pc.chd.skip = body.skip = False
        return (ProcObj(body, envt, para_list), False)
    def __str__(self):
        return "#<builtin opt lambda>"

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
        self.skip = None    # delay calcuation
    def __str__(self):
        return "#<AST Node>"
    def __expr__(self):
        return self.__str__()
    def print_(self):
        print \
            "======================" + "\n" + \
            "Obj: " + str(self.obj) + "\n" + \
            "Sib: " + str(self.sib) + "\n" + \
            "Chd: " + str(self.chd) + "\n" + \
            "======================" 

class RetAddr(object):
    def __init__(self, addr):
        self.addr = addr
    def get_addr(self):
        return self.addr
    def __str__(self):
        return "#<Return Address>"

class AbsSynTree(EvalObj):

    def to_obj(self, obj):
        if isinstance(obj, Node): return obj
        if obj is None: return obj
        try: return IntObj(obj)
        except Exception:
            try: return FloatObj(obj)
            except Exception: return IdObj(obj)
 
    def to_node(self, obj):
        if isinstance(obj, Node): return obj
        return Node(self.to_obj(obj))
        # else the obj is a string
       
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
                if len(lst) > 0:
                    root = Node(self.to_obj(lst[0]))
                    if len(lst) > 1:
                        root.chd = self.to_node(lst[1])
                        ref = root.chd
                        for i in lst[2:]:
                            ref.sib = self.to_node(i)
                            ref = ref.sib
                    stack[-1] = root
                else:
                    stack[-1] = self.to_node(None)
            else:
                stack.append(token)
            print stack

        self.tree = stack[0]

def is_obj(string):
    return isinstance(string, SyntaxObj)
def is_identifier(string):
    return isinstance(string, IdObj)
def is_leaf(node):
    return node.chd is None
def is_ret_addr(val):
    return isinstance(val, RetAddr)
def is_builtin_proc(val):
    return isinstance(val, BuiltinProcObj)
def is_special_opt(val):
    return isinstance(val, SpecialOptObj)
def is_user_defined_proc(val):
    return isinstance(val, ProcObj)

class Environment(object):
    def __init__(self, prev_envt = None):
        self.prev_envt = prev_envt
        self.binding = dict()
    def add_binding(self, name, eval_obj):
        self.binding[name] = eval_obj
    def get_obj(self, id_obj):
        if is_identifier(id_obj):
            ptr = self
            while ptr:
                try:
                    t = ptr.binding[id_obj.name]
                except KeyError:
                    t = None
                if t: return t
                ptr = ptr.prev_envt
            raise KeyError

        else:
            print "Not an id: " + str(id_obj)
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
        "lambda" : _builtin_lambda(),
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
        stack = [0] * 100   # Stack 
        ostack = [0] * 100     # Pending operators 
        pc = prog.tree  # Set to the root
        cont = Continuation(None, pc, None)
        envt = self.envt
        top = -1 # Stack top
        otop = -1

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

        def nxt_addr(ret_addr, otop):
            notop = otop
            if otop > -1 and ret_addr is ostack[notop]:
                notop -= 1
                res = ostack[notop].chd
                notop -= 1
            else:
                res = ret_addr.sib
            return (res, notop)


        def push(pc, top, otop):
            ntop = top
            notop = otop
            pc.print_()
            if is_leaf(pc):
                print "first"
                ntop += 1
                stack[ntop] = envt.get_obj(pc.obj)
                npc = pc.sib
                pc.print_()
                print "this val is: " + str(stack[ntop].val)
            else:
                print "second"
                ntop += 1
                stack[ntop] = RetAddr(pc) # Return address
                if isinstance(pc.obj, Node):
                    print "Operand need to be resolved!"
                    notop += 1
                    ostack[notop] = pc
                    notop += 1
                    ostack[notop] = pc.obj
                    pc.obj.print_() # Step in to resolve operator
                    npc = pc.obj
                else:
                    print "Getting operand: " + str(pc.obj.name)
                    ntop += 1
                    stack[ntop] = envt.get_obj(pc.obj)
                    if is_special_opt(stack[ntop]):
                        mask = stack[ntop].prepare(pc)
                        mask_eval(pc, mask)
                    npc = pc.chd
            return (npc, ntop, notop)

        print " Pushing..."
        print_stack()
        (pc, top, otop) = push(pc, top, otop)
        print_stack()
        print " Done...\n"
        while is_ret_addr(stack[0]):    # Still need to evaluate
            print "- Top: " + str(stack[top])
            print "- Pc at: " + str(pc)
            while pc and pc.skip: 
                print "skipping masked branch: " + str(pc)
                pc = pc.sib  # Skip the masked branches

            if pc is None:

                if top > 0 and is_ret_addr(stack[top - 1]) and \
                stack[top - 1].get_addr() is False:
                    stack[top - 1] = stack[top]
                    top -= 1
                    envt = cont.envt
                    (pc, otop) = nxt_addr(cont.pc, otop)
                    cont = cont.old_cont
                    # Revert to the original cont.
                else:
    
                    print "Poping..."
                    arg_list = list()
                    while not is_ret_addr(stack[top]):
                        arg_list = [stack[top]] + arg_list
                        top -= 1
                    # Top is now pointing to the return address
                    print "Arg List: " + str(arg_list)
                    opt = arg_list[0]
                    ret_addr = stack[top].get_addr()
    
                    if is_builtin_proc(opt):                # Built-in Procedures
                        print "builtin"
                        stack[top] = opt.call(arg_list[1:])
                        (pc, otop) = nxt_addr(ret_addr, otop)
    
                    elif is_special_opt(opt):               # Sepecial Operations
                        print "specialopt"
                        (res, flag) = opt.call(arg_list[1:], ret_addr, envt, cont)
                        if flag: # Need to call again
                            print "AGAIN with the mask: " + str(res)
                            mask_eval(ret_addr, res)
                            top += 1
                            pc = ret_addr.chd # Again
                        else:
                            stack[top] = res # Done
                            (pc, otop) = nxt_addr(ret_addr, otop)
                    elif is_user_defined_proc(opt):   # User-defined Procedures
                        ncont = Continuation(envt, ret_addr, cont)  # Create a new continuation
                        cont = ncont                          # Make chain
                        envt = Environment(opt.envt)         # New ENV and recover the closure
                        #TODO: Compare the arguments to the parameters
                        for i in xrange(1, len(arg_list)):
                            envt.add_binding(opt.para_list[i - 1].name, 
                                            arg_list[i])      # Create bindings
                        stack[top] = RetAddr(False)            # Mark the exit of the continuation
                        pc = opt.body                 # Get to the entry point
                
                print_stack()
                print "Poping done."
            else: 
                print " Pushing..."
                print_stack()
                (pc, top, otop) = push(pc, top, otop)
                print_stack()
                print " Done...\n"
        return stack[0]

import pdb
t = Tokenizor()
e = Evaluator()
e.envt.add_binding("x", IntObj(100))
#t.feed("(+ (- (* 10 2) (+ x 4)) (+ 1 2) (/ 25 5))")
#t.feed("(if (> 2 2) (if (> 2 1) 1 2) 3)")
t.feed("((lambda (x y) ((lambda (z) (+ z (* x y))) 10)) ((lambda (x y) (+ x y)) 3 2) 4)")
#t.feed("(lambda (x y) (* x x))")
a = AbsSynTree(t)
#a.tree.print_()
#a.tree.obj.print_()
print e.run_expr(a).val
##t.feed("((lambda (x) (x * x)) 2)")
#a = AbsSynTree(t)
#print e.run_expr(a).val
#a = AbsSynTree(t)
#print e.run_expr(a).val
