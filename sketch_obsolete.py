#! /bin/env python

class EvalObj(object):
    def __str__(self):
        return "#<Object>"

class UnspecObj(EvalObj):
    def __str__(self):
        return "#<Unspecified>"
    def ext_repr(self):
        return self.__str__()

class NumberObj(EvalObj):
    def __str__(selfl):
        return "#<Number>"

class IntObj(NumberObj):
    def __init__(self, num):
        self.val = int(num)
    def __str__(self):
        return "#<Integer>"
    def ext_repr(self):
        return str(self.val)

class FloatObj(NumberObj):
    def __init__(self, num):
        self.val = float(num)
    def __str__(self):
        return "#<Float>"
    def ext_repr(self):
        return str(self.val)

class BoolObj(EvalObj):
    def __init__(self, b):
        self.val = b
    def __str__(self):
        return "#<Boolean>"
    def ext_repr(self):
        if self.val:
            return "#t"
        else:
            return "#f"

class IdObj(EvalObj):
    def __init__(self, string):
        self.name = string
    def __str__(self):
        return "#<Identifier: " + self.name + ">"
    def get_name():
        return self.name

class OptObj(EvalObj):
    pass

class ProcObj(OptObj):
    def __init__(self, body, envt, para_list):
        self.body = body
        self.envt = envt
        self.para_list = para_list
    def ext_repr(self):
        return "#<Procedure>"
    def __str__(self):
        return self.ext_repr()

class SpecialOptObj(OptObj):
    def prepare(self, pc):
        pass
    def call(self, arg_list, pc, envt, cont):
        pass

class BuiltinProcObj():
    def __init__(self, f, name):
        self.handler = f
        self.name = name
    def ext_repr(self):
        return "#<Builtin Procedure: " + self.name + ">"
    def __str__(self):
        return self.ext_repr()
    def call(self, arg_list):
        return self.handler(arg_list)

# Convert an obj to boolean
def to_bool(obj):
    if obj.val is False:
        return BoolObj(False)
    else:
        return BoolObj(True)

# Mark all children of pc as flag
def _fill_marks(pc, flag):
    pc = pc.chd
    while pc:
        pc.skip = flag
        pc = pc.sib

class _builtin_if(SpecialOptObj):
    def prepare(self, pc):
        # TODO: check number of arguments 
        # Evaluate the condition first
        self.state = 0 # Prepared
        pc = pc.chd
        pc.skip = False
        pc.sib.skip = True
        if pc.sib.sib:
            pc.sib.sib.skip = True

    def pre_call(self, arg_list, pc, envt, cont):
        # Condition evaluated and the decision is made
        self.state = 1 
        if (to_bool(arg_list[0])).val:
            pc = pc.chd
            pc.skip = True
            pc.sib.skip = False
            if pc.sib.sib:
                # Eval the former
                pc.sib.sib.skip = True
            return (None, True) # Re-eval
        else:
            pc = pc.chd
            pc.skip = True
            pc.sib.skip = True
            if pc.sib.sib:
                # Eval the latter
                pc.sib.sib.skip = False
            return (None, True) # Re-eval

    def post_call(self, arg_list, pc, envt, cont):
        # Value already evaluated, so just return it
        return (arg_list[0], False)

    def call(self, arg_list, pc, envt, cont):
        if self.state == 0:
            return self.pre_call(arg_list, pc, envt, cont)
        else:
            return self.post_call(arg_list, pc, envt, cont)

    def ext_repr(self):
        return "#<Builtin Macro: if>"
    def __str__(self):
        return self.ext_repr()

class _builtin_lambda(SpecialOptObj):

    def prepare(self, pc):
        # TODO: check number of arguments 
        # Do not evaulate anything
        _fill_marks(pc, True)

    def call(self, arg_list, pc, envt, cont):
        para_list = list()              # paramter list
        par = pc.chd                    # Switch to the first parameter
        if par.obj.obj:                     # If there is at least one parameter
            para_list.append(par.obj.obj)
            if par.chd:                 # More paramters?
                par = par.chd
                while par: 
                    para_list.append(par.obj)
                    par = par.sib

        # Clear the flag to avoid side-effects (e.g. proc calling)
        _fill_marks(pc, False)

        pc = pc.chd.sib     # Move pc to procedure body
        #TODO: check body
        body = list()       # store a list of expressions inside <body>

        while pc:
            body.append(pc)
            pc.next = None  # Make each expression a orphan 
                            # in order to ease the exit checking
            pc = pc.sib

        return (ProcObj(body, envt, para_list), False)

    def ext_repr(self):
        return "#<Builtin Macro: lambda>"
    def __str__(self):
        return self.ext_repr()

class _builtin_define(SpecialOptObj):

    def prepare(self, pc):
        if is_arg(pc.chd):          # Simple value assignment
            pc.chd.skip = True      # Skip the identifier
            pc.chd.sib.skip = False
        else:                       # Procedure definition
            _fill_marks(pc, True)   # Skip all parts

    def call(self, arg_list, pc, envt, cont):
        # TODO: check identifier
        if is_arg(pc.chd):          # Simple value assignment
            id = pc.chd.obj             
            obj = arg_list[0]
        else:                       # Procedure definition
            id = pc.chd.obj.obj
            para_list = list()      # Parameter list
            par = pc.chd
            if par.chd:             # If there's at least one parameter
                par = par.chd
                while par:
                    para_list.append(par.obj)
                    par = par.sib

            # Clear the flag to avoid side-effects (e.g. proc calling)
            _fill_marks(pc, False)  
            pc = pc.chd.sib         # Move pc to procedure body
            #TODO: check body
            body = list()           # store a list of expressions inside <body>

            while pc:
                body.append(pc)
                pc.next = None
                pc = pc.sib

            obj = ProcObj(body, envt, para_list)
        
        envt.add_binding(id, obj)
        return (UnspecObj(), False)

    def ext_repr(self):
        return "#<Builtin Macro: define>"
    def __str__(self):
        return self.ext_repr()

class _builtin_set(SpecialOptObj):
    def prepare(self, pc):
        # TODO: check number of arguments 
        pc = pc.chd             
        pc.skip = True              # Skip the identifier
        pc.sib.skip = False  

    def call(self, arg_list, pc, envt, cont):
        id = pc.chd.obj
        if envt.has_obj(id):
            envt.add_binding(id, arg_list[0])
        return (UnspecObj(), False)

    def ext_repr(self):
        return "#<Builtin Macro: set!>"
    def __str__(self):
        return self.ext_repr()

class Tokenizor():

    def __init__(self):
        self.data = ""                  # string buffer
        self.tokenized = list()         # tokens

    def feed(self, data):               # Store the data in the buffer
        self.data = data

    def read(self): 
        if len(self.tokenized) == 0:        # no tokens available, let's produce
            if len(self.data) == 0:         # I'm hungry, feed me!
                return None
            self.tokenized = self.data.replace('(', '( ')\
                                    .replace(')', ' )')\
                                    .split()
            self.data = ""                  # Clear the buffer
            if len(self.tokenized) == 0:    # You feed me with the air, bastard!
                return None
        return self.tokenized.pop(0)

class Node(object):                     # AST Node
    def __init__(self, obj, sib):
        self.obj = obj
        self.sib = sib
        self.skip = None                # skip the current branch? (runtime)
        self.next = sib                 # next instruction         (runtime)

    def __str__(self):
        return "#<AST Node>"
    def __expr__(self):
        return self.__str__()

class ArgNode(Node):                    # AST Node (Argument)
    def __init__(self, obj, sib = None):
        super(ArgNode, self).__init__(obj, sib)
    def __str__(self):
        return "#<AST ArgNode>"
    def __expr__(self):
        return self.__str__()

    def print_(self):
        print \
            "======================" + "\n" + \
            "Obj: " + str(self.obj) + "\n" + \
            "Sib: " + str(self.sib) + "\n" + \
            "======================" 

class OptNode(Node):                    # AST Node (Operator)
    def __init__(self, obj, sib = None, chd = None):
        super(OptNode, self).__init__(obj, sib)
        self.chd = chd

    def __str__(self):
        return "#<AST OptNode>"
    def __expr__(self):
        return self.__str__()

    def print_(self):
        print \
            "======================" + "\n" + \
            "Obj: " + str(self.obj) + "\n" + \
            "Sib: " + str(self.sib) + "\n" + \
            "Chd: " + str(self.chd) + "\n" + \
            "======================" 

class RetAddr(object):                  # Return Adress (Refers to an AST Node)
    def __init__(self, addr):
        self.addr = addr
    def __str__(self):
        return "#<Return Address>"

class ASTGenerator(EvalObj):            # Abstract Syntax Tree Generator

    def to_obj(self, obj):              # Try to convert a string to EvalObj
        if isinstance(obj, Node): return obj
        try: return IntObj(obj)
        except Exception:
            try: return FloatObj(obj)
            except Exception: return IdObj(obj)
 
    def to_node(self, obj):             # Try to convert an EvalObj to AST Node
        if isinstance(obj, Node): return obj
        return ArgNode(obj)
       
    def __init__(self, stream):
        self.stream = stream
        self.stack = list()  # Empty stack

    def absorb(self):
        stack = self.stack
        while True:
            if len(stack) > 0 and stack[0] != '(':
                return self.to_node(stack.pop(0))   # An AST is constructed
            token = self.stream.read()              # Read a new token 
            if token is None: return None           # Feed me!
            if token == '(':
                stack.append(token) 
            elif token == ')':                      # A list is enclosed
                lst = list()
                while stack[-1] != '(':
                    lst = stack[-1:] + lst
                    del stack[-1]
                if len(lst) > 0:                    # At least one elem
                    root = OptNode(lst[0])          # The operator in the list
                    # Collect the operands
                    if len(lst) > 1:
                        root.chd = lst[1]
                        ref = root.chd
                        for i in lst[2:]:
                            ref.sib = i
                            ref.next = ref.sib
                            ref = ref.sib
                    stack[-1] = root
                else:                               # Null list
                    stack[-1] = OptNode(ArgNode(None))
            else:
                stack.append(ArgNode(self.to_obj(token)))    # Found an EvalObj

def is_id(string):
    return isinstance(string, IdObj)
def is_arg(node):
    return isinstance(node, ArgNode)
def is_ret_addr(val):
    return isinstance(val, RetAddr)
def is_builtin_proc(val):
    return isinstance(val, BuiltinProcObj)
def is_special_opt(val):
    return isinstance(val, SpecialOptObj)
def is_user_defined_proc(val):
    return isinstance(val, ProcObj)

class Environment(object):                      # Store all bindings
    def __init__(self, prev_envt = None):
        self.prev_envt = prev_envt
        self.binding = dict()

    def add_binding(self, id_obj, eval_obj):    # Bind id_obj to eval_obj
        self.binding[id_obj.name] = eval_obj

    def get_obj(self, id_obj):
        if is_id(id_obj):                # Resolve the id
            ptr = self
            while ptr:                   # Lookup the id in the chain
                try:
                    return ptr.binding[id_obj.name]
                except KeyError:
                    ptr = ptr.prev_envt
            raise KeyError
        else:
            return id_obj                # id_obj is inherently an EvalObj

    def has_obj(self, id_obj):
        ptr = self
        while ptr:
            try:
                t = ptr.binding[id_obj.name]
                return True
            except KeyError: 
                ptr = ptr.prev_envt
        return False

class Continuation(object):             # Store the state of the interpreter
    def __init__(self, envt, pc, old_cont, proc_body, body_cnt = 0):    
        self.envt = envt                # envt pointer
        self.pc = pc                    # pc pointer
        self.old_cont = old_cont        # previous state
        self.proc_body = proc_body      # procedure expression list
        self.body_cnt = 0               # how many exp have been evaluated
       
# Miscellaneous builtin procedures #
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

def _builtin_display(arg_list):
    print "Display: " + arg_list[0].ext_repr()
    return UnspecObj()
# Miscellaneous builtin procedures #

_default_mapping = {
        IdObj("+") : BuiltinProcObj(_builtin_plus, "+"),
        IdObj("-") : BuiltinProcObj(_builtin_minus, "-"),
        IdObj("*") : BuiltinProcObj(_builtin_times, "*"),
        IdObj("/") : BuiltinProcObj(_builtin_div, "/"),
        IdObj("<") : BuiltinProcObj(_builtin_lt, "<"),
        IdObj(">") : BuiltinProcObj(_builtin_gt, ">"),
        IdObj("=") : BuiltinProcObj(_builtin_eq, "="),
        IdObj("display") : BuiltinProcObj(_builtin_display, "display"),
        IdObj("lambda") : _builtin_lambda(),
        IdObj("if") : _builtin_if(),
        IdObj("define") : _builtin_define(),
        IdObj("set!") : _builtin_set()}

class Evaluator(object):

    def _add_builtin_routines(self, envt):
        for sym in _default_mapping:
            envt.add_binding(sym, _default_mapping[sym])

    def __init__(self):
        self.envt = Environment() # Top-level Env
        self._add_builtin_routines(self.envt)
    def run_expr(self, prog):
        stack = [0] * 100       # Eval Stack 
        ostack = [0] * 100      # Operator to be evaluated 

        top = -1                # stack top
        otop = -1               # ostack top

        pc = prog               # pc register
        cont = None             # continuation register
        envt = self.envt        # environment register

        def print_stack():
            print '++++++++++STACK++++++++'
            if len(stack) > 0:
                for i in range(0, top + 1):
                    print stack[i]
            print '----------STACK--------'
        
        def next_addr(ret_addr, otop):  # Get the next instruction (after returning)
            notop = otop
            if otop > -1 and ret_addr is ostack[notop]:
                # If the operator is evaluated successfully
                # pc should point to its operand
                notop -= 1
                res = ostack[notop].chd
                notop -= 1
            else:
                # Normal situation: move to the next operand
                res = ret_addr.next
            return (res, notop)


        def push(pc, top, otop):        # Push EvalObj to the stack
            ntop = top
            notop = otop
            if is_arg(pc):              # Pure operand 
                ntop += 1
                stack[ntop] = envt.get_obj(pc.obj)  # Try to find the binding
                npc = pc.next           # Move to the next instruction
            else:                       # Found an Operator
                ntop += 1
                stack[ntop] = RetAddr(pc)       # Push return address
                if is_arg(pc.obj):              # Getting operator
                    ntop += 1
                    stack[ntop] = envt.get_obj(pc.obj.obj)
                    if is_special_opt(stack[ntop]):
                        stack[ntop].prepare(pc)
                    npc = pc.chd
                else:                           # Operator need to be evaluated
                    notop += 1
                    ostack[notop] = pc
                    notop += 1
                    ostack[notop] = pc.obj
                    npc = pc.obj

            return (npc, ntop, notop)

        (pc, top, otop) = push(pc, top, otop)

        while is_ret_addr(stack[0]):    # Still need to evaluate
            while pc and pc.skip: 
                pc = pc.next            # Skip the masked branches

            if pc is None:              # All arguments are evaluated, exiting
               arg_list = list()
               # Collect all arguments
               while not is_ret_addr(stack[top]):
                   arg_list = [stack[top]] + arg_list
                   top -= 1
               opt = arg_list[0]            # the operator
               ret_addr = stack[top].addr   # Return address

                # Fake return (one of the expressions are evaluated)
               if ret_addr is False:        
                    body = cont.proc_body
                    cont.body_cnt += 1
                    ncur = cont.body_cnt
                    if ncur == len(body):   # All exps in the body are evaled
                        stack[top] = arg_list[0]
                        envt = cont.envt
                        (pc, otop) = next_addr(cont.pc, otop)
                        cont = cont.old_cont
                    else:
                        pc = body[ncur]     # Load the next exp
                    continue
                    # Revert to the original cont.
   
               if is_builtin_proc(opt):                 # Built-in Procedures
                   stack[top] = opt.call(arg_list[1:])
                   (pc, otop) = next_addr(ret_addr, otop)
    
               elif is_special_opt(opt):                # Sepecial Operations
                   (res, flag) = opt.call(arg_list[1:], ret_addr, envt, cont)
                   if flag:                             # Need to call again
                       top += 1
                       pc = ret_addr.chd                # Invoke again
                   else:
                       stack[top] = res                 # Done
                       (pc, otop) = next_addr(ret_addr, otop)

               elif is_user_defined_proc(opt):          # User-defined Procedures
                   # Create a new continuation
                   ncont = Continuation(envt, ret_addr, cont, opt.body)  
                   cont = ncont                         # Add to the cont chain
                   envt = Environment(opt.envt)         # Create local env and recall the closure
                   #TODO: Compare the arguments to the parameters
                   for i in xrange(1, len(arg_list)):
                       envt.add_binding(opt.para_list[i - 1], arg_list[i])      
                                                        # Create bindings
                   stack[top] = RetAddr(False)          # Continuation mark 
                   pc = opt.body[0]                     # Move to the proc entry point
            else: 
                (pc, top, otop) = push(pc, top, otop)

        return stack[0]

t = Tokenizor()
e = Evaluator()

import sys, pdb

a = ASTGenerator(t)
while True:
    sys.stdout.write("Sonsi> ")
    while True:
        exp = a.absorb()
        if exp: break
        cmd = sys.stdin.readline()
        t.feed(cmd)
    try:
        print e.run_expr(exp).ext_repr()
    except Exception as exc:
        print exc
