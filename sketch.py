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

class StringObj(EvalObj):
    def __init__(self, string):
        self.val = string
    def __str__(self):
        return "#<String>"
    def ext_repr(self):
        return self.val

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

def to_bool(obj):
    if obj.val is False:
        return BoolObj(False)
    else:
        return BoolObj(True)

class _builtin_if(SpecialOptObj):
    def prepare(self, pc):
        self.state = 0 # prepare
        # TODO: check number of arguments 
        pc = pc.chd
        pc.skip = False
        pc.sib.skip = True
        if pc.sib.sib:
            pc.sib.sib.skip = True
        # Delay the calculation
    def pre_call(self, arg_list, pc, envt, cont):
        self.state = 1 # calling
        # print "Received if signal: " + str(arg_list[0].val)
        # print "And it is regared as: " + str(to_bool(arg_list[0]).val)
        if (to_bool(arg_list[0])).val:
            pc = pc.chd
            pc.skip = True
            pc.sib.skip = False
            if pc.sib.sib:
                pc.sib.sib.skip = True
            return (None, True) # Re-eval
        else:
            pc = pc.chd
            pc.skip = True
            pc.sib.skip = True
            if pc.sib.sib:
                pc.sib.sib.skip = False
            return (None, True) # Re-eval
    def post_call(self, arg_list, pc, envt, cont):
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
    def _clear_marks(self, pc, flag):
        pc = pc.chd
        while pc:
            pc.skip = flag
            pc = pc.sib

    def prepare(self, pc):
        # TODO: check number of arguments 
        self._clear_marks(pc, True)

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
        self._clear_marks(pc, False)
        pc = pc.chd.sib
        #TODO: check body
        body = list()
        while pc:
            body.append(pc)
            t = pc.sib
            pc.next = None # Detach
            pc = t
            # Add exps
        return (ProcObj(body, envt, para_list), False)
    def ext_repr(self):
        return "#<Builtin Macro: lambda>"
    def __str__(self):
        return self.ext_repr()

class _builtin_define(SpecialOptObj):
    def prepare(self, pc):
        # TODO: check number of arguments 
        pc = pc.chd
        pc.skip = True
        pc.sib.skip = False

    def call(self, arg_list, pc, envt, cont):
        # TODO: check identifier
        id = pc.chd.obj
        envt.add_binding(id, arg_list[0])
        return (UnspecObj(), False)

    def ext_repr(self):
        return "#<Builtin Macro: define>"
    def __str__(self):
        return self.ext_repr()

class _builtin_set(SpecialOptObj):
    def prepare(self, pc):
        # TODO: check number of arguments 
        pc = pc.chd
        pc.skip = True
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

class IdObj(EvalObj):
    def __init__(self, string):
        self.name = string
    def __str__(self):
        return "#<Identifier: " + self.name + ">"
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
            if len(self.tokenized) == 0:
                return None
        return self.tokenized.pop(0)

class Node(object):
    def __init__(self, obj, sib):
        self.obj = obj
        self.sib = sib
        self.skip = None    # delay calcuation
        self.next = sib
    def __str__(self):
        return "#<AST Node>"
    def __expr__(self):
        return self.__str__()

class ArgNode(Node):
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

class OptNode(Node):
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

class RetAddr(object):
    def __init__(self, addr):
        self.addr = addr
    def __str__(self):
        return "#<Return Address>"

class ASTree(EvalObj):

    def to_obj(self, obj):
        if isinstance(obj, Node): return obj
        try: return IntObj(obj)
        except Exception:
            try: return FloatObj(obj)
            except Exception: return IdObj(obj)
 
    def to_node(self, obj):
        if isinstance(obj, Node): return obj
        return ArgNode(obj)
        # else the obj is a string
       
    def __init__(self, stream):
        self.stream = stream
        self.stack = list()  # Empty stack

    def absorb(self):
        stack = self.stack
        while True:
            if len(stack) > 0 and stack[0] != '(':
                return self.to_node(stack.pop(0)) # Got an expression
            token = self.stream.read()
            if token is None: return None
            if token == '(':
                stack.append(token) 
            elif token == ')':
                lst = list()
                while stack[-1] != '(':
                    lst = stack[-1:] + lst
                    del stack[-1]
                if len(lst) > 0:
                    root = OptNode(lst[0])
                    if len(lst) > 1:
                        root.chd = self.to_node(lst[1])
                        ref = root.chd
                        for i in lst[2:]:
                            ref.sib = self.to_node(i)
                            ref.next = ref.sib
                            ref = ref.sib
                    stack[-1] = root
                else:
                    stack[-1] = self.to_node(None)
            else:
                stack.append(self.to_obj(token))

def is_obj(string):
    return isinstance(string, EvalObj)
def is_identifier(string):
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

class Environment(object):
    def __init__(self, prev_envt = None):
        self.prev_envt = prev_envt
        self.binding = dict()

    def add_binding(self, id_obj, eval_obj):
        self.binding[id_obj.name] = eval_obj

    def has_obj(self, id_obj):
        ptr = self
        while ptr:
            try:
                t = ptr.binding[id_obj.name]
                return True
            except KeyError: 
                ptr = ptr.prev_envt
        return False

    def get_obj(self, id_obj):
        if is_identifier(id_obj):
            ptr = self
            while ptr:
                try:
                    return ptr.binding[id_obj.name]
                except KeyError:
                    ptr = ptr.prev_envt
            raise KeyError
        else:
            # print "Not an id: " + str(id_obj)
            return id_obj

class Continuation(object):
    def __init__(self, envt, pc, old_cont, proc_body, proc_body_cur = 0):    
        self.envt = envt
        self.pc = pc
        self.old_cont = old_cont
        self.proc_body = proc_body
        self.proc_body_cur = 0
        
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

def _builtin_display(arg_list):
    print "Display: " + arg_list[0].ext_repr()
    return UnspecObj()

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
        stack = [0] * 100   # Stack 
        ostack = [0] * 100     # Pending operators 
        pc = prog       # Set to the root
        cont = Continuation(None, pc, None, None)
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
                # print i
                # print pc
                pc.skip = not i
                pc = pc.sib

        def nxt_addr(ret_addr, otop):
            notop = otop
            if otop > -1 and ret_addr is ostack[notop]:
                notop -= 1
                res = ostack[notop].chd
                notop -= 1
            else:
                res = ret_addr.next
            return (res, notop)


        def push(pc, top, otop):
            ntop = top
            notop = otop
            if is_arg(pc):
                # print "first"
                ntop += 1
                if pc.skip:
                    new_obj = pc.obj
                else:
                    new_obj = envt.get_obj(pc.obj)
                stack[ntop] = new_obj
                npc = pc.next
                # pc.print_()
                #print "this val is: " + str(stack[ntop].val)
            else:
                # print "second"
                ntop += 1
                stack[ntop] = RetAddr(pc) # Return address
                if isinstance(pc.obj, Node):
                    # print "Operator need to be resolved!"
                    notop += 1
                    ostack[notop] = pc
                    notop += 1
                    ostack[notop] = pc.obj
                    # pc.obj.print_() # Step in to resolve operator
                    npc = pc.obj
                else:
                    # print "Getting operator: " + str(pc.obj.name)
                    ntop += 1
                    stack[ntop] = envt.get_obj(pc.obj)
                    if is_special_opt(stack[ntop]):
                        stack[ntop].prepare(pc)
                        #mask = stack[ntop].prepare()
                        #mask_eval(pc, mask)
                    npc = pc.chd
            return (npc, ntop, notop)

        # print " Pushing..."
        # print_stack()
        (pc, top, otop) = push(pc, top, otop)
        # print_stack()
        # print " Done...\n"
        while is_ret_addr(stack[0]):    # Still need to evaluate
            # print "- Top: " + str(stack[top])
            # print "- Pc at: " + str(pc)
            while pc and pc.skip: 
                # print "skipping masked branch: " + str(pc)
                pc = pc.next  # Skip the masked branches

            if pc is None:

               # print "Poping..."
               arg_list = list()
               while not is_ret_addr(stack[top]):
                   arg_list = [stack[top]] + arg_list
                   top -= 1
               # Top is now pointing to the return address
               # print "Arg List: " + str(arg_list)
               opt = arg_list[0]
               ret_addr = stack[top].addr

               if ret_addr is False: # Fake return
                    body = cont.proc_body
                    cont.proc_body_cur += 1
                    ncur = cont.proc_body_cur
                    if ncur == len(body):   # All exps in the body are evaled
                        stack[top] = arg_list[0]
                        envt = cont.envt
                        (pc, otop) = nxt_addr(cont.pc, otop)
                        cont = cont.old_cont
                    else:
                        pc = body[ncur]     # Load the next exp

                    continue
                    # Revert to the original cont.
   
               if is_builtin_proc(opt):                # Built-in Procedures
                   # print "builtin"
                   stack[top] = opt.call(arg_list[1:])
                   (pc, otop) = nxt_addr(ret_addr, otop)
    
               elif is_special_opt(opt):               # Sepecial Operations
                   # print "specialopt"
                   (res, flag) = opt.call(arg_list[1:], ret_addr, envt, cont)
                   if flag: # Need to call again
                       # print "AGAIN with the mask: " + str(res)
                       # mask_eval(ret_addr, res)
                       top += 1
                       pc = ret_addr.chd # Again
                   else:
                       stack[top] = res # Done
                       (pc, otop) = nxt_addr(ret_addr, otop)
               elif is_user_defined_proc(opt):   # User-defined Procedures
                   ncont = Continuation(envt, ret_addr, cont, opt.body)  # Create a new continuation
                   cont = ncont                          # Make chain
                   envt = Environment(opt.envt)         # New ENV and recover the closure
                   #TODO: Compare the arguments to the parameters
                   for i in xrange(1, len(arg_list)):
                       envt.add_binding(opt.para_list[i - 1], 
                                       arg_list[i])      # Create bindings
                   stack[top] = RetAddr(False)            # Mark the exit of the continuation
                   pc = opt.body[0]                 # Get to the entry point
                
                # print_stack()
                # print "Poping done."
            else: 
                # print " Pushing..."
                # print_stack()
                (pc, top, otop) = push(pc, top, otop)
                # print_stack()
                # print " Done...\n"
        return stack[0]

t = Tokenizor()
e = Evaluator()

import sys, pdb

# ins_set = ("(define g (lambda (x) (if (= x 5) 0 ((lambda () (display x) (g (+ x 1)))))))",
#            "(g 0)")
# ins_set = ("(define g (lambda (x) (define y 10) (+ x y)))",
#             "g",
#             "(g 1)",
#             "y")
#ins_set = ("((lambda () (display 2)))",)
#ins_set = ("((lambda (x y) (+ x y)) 1 2)",)
#ins_set = ("(+ 1 2)",)
# t.feed(ins_set)
# a = ASTree(t)
# print a.tree.chd.print_()
#for ins in ins_set:
#     print "TEST"
#     t.feed(ins)
#     print e.run_expr(ASTree(t)).ext_repr()
#

a = ASTree(t)
while True:
    sys.stdout.write("Syasi> ")
    while True:
        exp = a.absorb()
        if exp: break
        cmd = sys.stdin.readline()
        t.feed(cmd)
    print e.run_expr(exp).ext_repr()
