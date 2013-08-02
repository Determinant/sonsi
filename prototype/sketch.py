#! /bin/env python

class EvalObj(object):
    def prepare(self, pc):
        pass
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

class SymObj(EvalObj):
    def __init__(self, string):
        self.name = string
    def __str__(self):
        return "#<Symbol: " + self.name + ">"
    def get_name():
        return self.name

class OptObj(EvalObj):
    pass

class ProcObj(OptObj):
    def __init__(self, body, envt, para_list):
        self.body = body
        self.envt = envt
        self.para_list = para_list
    def call(self, arg_list, envt, cont, stack, top, ret_addr):
        # Create a new continuation
        ncont = Continuation(envt, ret_addr, cont, self.body)  
        cont = ncont                         # Add to the cont chain
        envt = Environment(self.envt)        # Create local env and recall the closure
        #TODO: Compare the arguments to the parameters
        for i in xrange(1, len(arg_list)):
            envt.add_binding(self.para_list[i - 1], arg_list[i])      
                                             # Create bindings
        stack[top] = RetAddr(False)          # Continuation mark 
        pc = self.body[0]                         # Move to the proc entry point
        return (pc, top, envt, cont)

    def ext_repr(self):
        return "#<Procedure>"
    def __str__(self):
        return self.ext_repr()

class SpecialOptObj(OptObj):
        pass

class BuiltinProcObj(OptObj):
    def __init__(self, f, name):
        self.handler = f
        self.name = name
    def ext_repr(self):
        return "#<Builtin Procedure: " + self.name + ">"
    def __str__(self):
        return self.ext_repr()
    def call(self, arg_list, envt, cont, stack, top, ret_addr):
        stack[top] = self.handler(arg_list[1:])
        return (ret_addr.func.next, top, envt, cont)

# Convert an obj to boolean
def to_bool(obj):
    if obj.val is False:
        return BoolObj(False)
    else:
        return BoolObj(True)

# Mark all children of pc as flag
def _fill_marks(pc, flag):
    pc = pc.cdr
    while not (pc is empty_list):
        pc.func.skip = flag
        pc = pc.cdr

class _builtin_if(SpecialOptObj):
    def prepare(self, pc):
        # TODO: check number of arguments 
        # Evaluate the condition first
        self.state = 0 # Prepared
        pc = pc.cdr
        pc.func.skip = False
        pc.cdr.func.skip = True
        if not (pc.cdr.cdr is empty_list):
            pc.cdr.cdr.func.skip = True

    def pre_call(self, arg_list, pc, envt, cont):
        pc = pc.car
        # Condition evaluated and the decision is made
        self.state = 1 
        if (to_bool(arg_list[1])).val:
            pc = pc.cdr
            pc.func.skip = True
            pc.cdr.func.skip = False
            if not (pc.cdr.cdr is empty_list):
                # Eval the former
                pc.cdr.cdr.func.skip = True
        else:
            pc = pc.cdr
            pc.func.skip = True
            pc.cdr.func.skip = True
            if not (pc.cdr.cdr is empty_list):
                # Eval the latter
                pc.cdr.cdr.func.skip = False

    def post_call(self, arg_list, pc, envt, cont):
        # Value already evaluated, so just return it
        return arg_list[1]

    def call(self, arg_list, envt, cont, stack, top, ret_addr):
        if self.state == 0:
            self.pre_call(arg_list, ret_addr, envt, cont)
            top += 1
            pc = ret_addr.car.func.next     # Invoke again
        else:
            stack[top] = self.post_call(arg_list, ret_addr, envt, cont)
            pc = ret_addr.func.next
        return (pc, top, envt, cont)

    def ext_repr(self):
        return "#<Builtin Macro: if>"
    def __str__(self):
        return self.ext_repr()

class _builtin_lambda(SpecialOptObj):

    def prepare(self, pc):
        # TODO: check number of arguments 
        # Do not evaulate anything
        _fill_marks(pc, True)

    def call(self, arg_list, envt, cont, stack, top, ret_addr):
        pc = ret_addr.car
        para_list = list()              # paramter list TODO: use Cons to represent list
        par = pc.cdr.car                # Switch to the first parameter
        while not (par is empty_list): 
            para_list.append(par.car)
            par = par.cdr

        # Clear the flag to avoid side-effects (e.g. proc calling)
        _fill_marks(pc, False)

        pc = pc.cdr.cdr     # Move pc to procedure body
        #TODO: check body
        body = list()       # store a list of expressions inside <body> TODO: Cons

        while not (pc is empty_list):
            body.append(pc)
            pc.func.next = None # Make each expression a orphan 
                                # in order to ease the exit checking
            pc = pc.cdr

        stack[top] = ProcObj(body, envt, para_list)
        return (ret_addr.func.next, top, envt, cont)

    def ext_repr(self):
        return "#<Builtin Macro: lambda>"
    def __str__(self):
        return self.ext_repr()

class _builtin_define(SpecialOptObj):

    def prepare(self, pc):
        if is_arg(pc.cdr):              # Simple value assignment
            pc.cdr.func.skip = True     # Skip the identifier
            pc.cdr.cdr.func.skip = False
        else:                       # Procedure definition
            _fill_marks(pc, True)   # Skip all parts

    def call(self, arg_list, envt, cont, stack, top, ret_addr):
        pc = ret_addr.car
        # TODO: check identifier
        if is_arg(pc.cdr):          # Simple value assignment
            id = pc.cdr.car             
            obj = arg_list[1]
        else:                       # Procedure definition
            id = pc.cdr.car.car
            para_list = list()      # Parameter list
            par = pc.cdr.car
            if not (par.cdr is empty_list):
                # If there's at least one parameter
                par = par.cdr
                while not (par is empty_list):
                    para_list.append(par.car)
                    par = par.cdr

            # Clear the flag to avoid side-effects (e.g. proc calling)
            _fill_marks(pc, False)  
            pc = pc.cdr.cdr         # Move pc to procedure body
            #TODO: check body
            body = list()           # store a list of expressions inside <body>

            while not (pc is empty_list):
                body.append(pc)
                pc.func.next = None
                pc = pc.cdr

            obj = ProcObj(body, envt, para_list)
        
        envt.add_binding(id, obj)
        stack[top] = UnspecObj()
        return (ret_addr.func.next, top, envt, cont)

    def ext_repr(self):
        return "#<Builtin Macro: define>"
    def __str__(self):
        return self.ext_repr()

class _builtin_set(SpecialOptObj):
    def prepare(self, pc):
        # TODO: check number of arguments 
        pc.cdr.func.skip = True     # Skip the identifier
        pc.cdr.cdr.func.skip = False  

    def call(self, arg_list, envt, cont, stack, top, ret_addr):
        pc = ret_addr.car
        id = pc.cdr.car
        if envt.has_obj(id):
            envt.add_binding(id, arg_list[1])
        stack[top] = UnspecObj()
        return (ret_addr.func.next, top, envt, cont)

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

class Prog(object):
    def __init__(self, skip = False, next = None):
        self.skip = skip
        self.next = next

class Data(object):
    pass

class Cons(object):                     # Construction
    def __init__(self, car, cdr, func = Data()):
        self.car = car
        self.cdr = cdr
        self.func = func
    def ext_repr(self):
        return "#<Cons>"
    def __str__(self):
        return self.ext_repr()
    def print_(self):
        print "car: " + str(self.car) + "\n" + \
              "cdr: " + str(self.cdr) + "\n"

class EmptyList(object):
    def ext_repr(self):
        return "()"
    def __str__(self):
        return self.ext_repr()

empty_list = EmptyList()

class RetAddr(object):                  # Return Adress (Refers to an Cons)
    def __init__(self, addr):
        self.addr = addr
    def __str__(self):
        return "#<Return Address>"

class ASTGenerator(EvalObj):            # Abstract Syntax Tree Generator

    def to_obj(self, obj):              # Try to convert a string to EvalObj
        try: return IntObj(obj)
        except Exception:
            try: return FloatObj(obj)
            except Exception: return SymObj(obj)
 
    def __init__(self, stream):
        self.stream = stream
        self.stack = list()  # Empty stack

    def absorb(self):
        stack = self.stack
        while True:
            if len(stack) > 0 and stack[0] != '(':
                return Cons(stack.pop(0), empty_list, Prog(0))   
                                                    # An AST is constructed
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
                    root = Cons(lst[0], None, Prog())          # The operator in the list
                    # Collect the operands
                    ref = root
                    for i in lst[1:]:
                        ref.cdr = Cons(i, None, Prog())
                        ref.func.next = ref.cdr
                        ref = ref.cdr
                    ref.cdr = empty_list
                    stack[-1] = root
                else:                               # Null list
                    stack[-1] = empty_list
            else:
                stack.append(self.to_obj(token))    # Found an EvalObj

def is_id(string):
    return isinstance(string, SymObj)
def is_arg(pc):
    return isinstance(pc.car, EvalObj)
def is_ret_addr(val):
    return isinstance(val, RetAddr)

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
        SymObj("+") : BuiltinProcObj(_builtin_plus, "+"),
        SymObj("-") : BuiltinProcObj(_builtin_minus, "-"),
        SymObj("*") : BuiltinProcObj(_builtin_times, "*"),
        SymObj("/") : BuiltinProcObj(_builtin_div, "/"),
        SymObj("<") : BuiltinProcObj(_builtin_lt, "<"),
        SymObj(">") : BuiltinProcObj(_builtin_gt, ">"),
        SymObj("=") : BuiltinProcObj(_builtin_eq, "="),
        SymObj("display") : BuiltinProcObj(_builtin_display, "display"),
        SymObj("lambda") : _builtin_lambda(),
        SymObj("if") : _builtin_if(),
        SymObj("define") : _builtin_define(),
        SymObj("set!") : _builtin_set()}

class Evaluator(object):

    def _add_builtin_routines(self, envt):
        for sym in _default_mapping:
            envt.add_binding(sym, _default_mapping[sym])

    def __init__(self):
        self.envt = Environment() # Top-level Env
        self._add_builtin_routines(self.envt)
    def run_expr(self, prog):
        stack = [0] * 100       # Eval Stack 
        top = -1                # stack top

        pc = prog               # pc register
        cont = None             # continuation register
        envt = self.envt        # environment register

        def push(pc, top):        # Push EvalObj to the stack
            ntop = top
            if is_arg(pc):        # Pure operand 
                ntop += 1
                stack[ntop] = envt.get_obj(pc.car)  # Try to find the binding
                stack[ntop].prepare(pc)
                npc = pc.func.next      # Move to the next instruction
            else:                       # Found an Operator
                ntop += 1
                stack[ntop] = RetAddr(pc)           # Push return address
                npc = pc.car

            return (npc, ntop)

        (pc, top) = push(pc, top)

        while is_ret_addr(stack[0]):    # Still need to evaluate
            while pc and pc.func.skip: 
                pc = pc.func.next       # Skip the masked branches

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
                        pc = cont.pc.func.next
                        cont = cont.old_cont
                    else:
                        pc = body[ncur]     # Load the next exp
                    # Revert to the original cont.
               else:
                    (pc, top, envt, cont) = opt.call(arg_list, envt, cont, 
                                                    stack, top, ret_addr)
            else: 
                (pc, top) = push(pc, top)

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
