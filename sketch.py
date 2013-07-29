class SyntaxObj(object):
    pass

class EvalObj(SyntaxObj):
    def __str__(self):
        return "#<Object>"

class ValObj(EvalObj):
    def __str__(self):
        return "#<Value>"

class NumberObj(ValObj):
    def __str__():
        return "#<Number>"

class IntObj(NumberObj):
    def __init__(self, num):
        self.val = int(num)
    def __str__():
        return "#<Integer>"

class FloatObj(NumberObj):
    def __init__(self, num):
        self.val = float(num)
    def __str__():
        return "#<Float>"

class StringObj(ValObj):
    def __init__(self, string):
        self.val = string

    def __str__():
        return "#<String>"

class ProcObj(ValObj):
    def __init__(self, env, prog):
        self.val = proc

    def __str__():
        return "#<Procedure>"

class IdObj(SyntaxObj):
    def __init__(self, string):
        self.name = string
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

class AST(EvalObj):

    class Node(object):
        def __init__(self, syn_obj, sib = None, chd = None):
            self.obj = syn_obj
            self.sib = sib
            self.chd = chd
        def __str__(self):
            return  \
            "Obj: " + str(self.obj) + " " + \
            "Sib: " + str(self.sib) + " " + \
            "Chd: " + str(self.chd)

    def to_node(self, obj):
        if isinstance(obj, self.Node): return obj
        # else the obj is a string
        try: return self.Node(IntObj(obj))
        except ValueError:
            try: return self.Node(FloatObj(obj))
            except ValueError: return self.Node(IdObj(obj))

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
                root = self.to_node(lst[0])
                if len(lst) > 1:
                    root.chd = self.to_node(lst[1])
                    ref = root.chd
                    for i in lst[2:]:
                        ref.sib = self.to_node(i)
                        ref = ref.sib
                stack[-1] = root
            else:
                stack.append(token)
        self.tree = stack

class Environment(object):
    def __init__(self):
        self.binding = dict()
    def add_entry(id_obj, eval_obj):
        self.binding[id_obj] = eval_obj
    def get_obj(id_obj):
        return self.binding[id_obj]

class Continuation(object):
    def __init__(self, envt, pc, old_cont):    
        self.envt = envt
        self.pc = pc
        self.old_cont = old_cont
    def get_envt(self):
        return self.envt
    def get_cont(self):
        return self.cont

class Evaluator(object):
    def __init__(self):
        self.envt = Environment()
        self.cont = None
        self.pc = None
    def run(self, prog):
        self.stack = list() # object stack 
        self.flag = list() # call flag  True for call False for value
        self.pc = prog  # Set to the root
        self.top = 0 # Stack top

        while self.flag[0]:  # Still need to evaluate
            pass
             

t = Tokenizor()
t.feed("(lambda (x) (x * x))")
a = AST(t)
root = a.tree
print root.__str__()
