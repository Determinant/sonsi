Sonsi: Stupid and Obvious Non-recursive Scheme Interpreter
==========================================================

最终版的设计和特性

Model
-----
由于Scheme允许first-class procedures，我将所有参与求值的对象都抽象成 ``EvalObj`` ，然后再从中派生出较为具体的子类：

- EvalObj

  - BoolObj
  - CharObj
  - Container

    - Continuation
    - Environment
    - OptObj
    - PromObj
    - Pair
    - VecObj
  - NumObj
  - StrObj
  - SymObj
  - UnspecObj

其中OptObj描述了统一的算符接口，即s-expression中application的第一个的元素 ``opt``

::

    (opt arg_1, arg_2, arg_3 ... arg_n)

而OptObj又向下进行细分，有用户定义函数类 ``ProcObj`` , 简单内置函数类 ``BuiltinProcObj`` 以及特殊操作符类 ``SpecialOptObj`` 。
三者各自实现 ``OptObj`` 要求的界面

::

    vrtual Pair *callPair *args, Environment * &envt, Continuation * &cont, EvalObj ** &top_ptr, Pair *pc);

即根据求值器当前的状态以及传入的参数，计算出值后自行压栈，并返回下一步的PC(program counter)位置。
之所以不直接返回计算出的值，而是让类实现自行进行较为低层的入栈，是因为三个不同类型算符的特殊性。

像简单内置函数仅仅根据已经算好的各个参数计算出值，所以所有这类实现都是具有相同的routine，即传入参数，计算得值，再压回栈。因此没有必要从 ``BuiltinProcObj`` 根据每个要实现的built-in procedure派生一个类，而是共同采用 ``BuiltinProcObj`` 的壳子，装入不同的evaluation routine（类似回调函数）即可。所以繁琐的栈操作在 ``BuiltinProcObj`` 实现后，每次提供一个evaluation routine就能生成一个解决相应计算任务的实例，非常方便。
     
而较为复杂的 ``SpecialOptObj`` 则不同。例如 ``if`` 语句的行为模式就与 ``lambda`` 和 ``define`` 存在不小的差异，而且与普通procedure不同，它们是built-in macros，这也就是将call函数接口设计得较为generic的原因。以 ``if`` 举例，我从 ``SpecialOptObj`` 派生出一个针对 ``if`` 的类 ``SpecialOptIf`` ，它的 ``call`` 函数实现不再那么单纯。而是根据当前 ``if`` 的执行阶段决定应该做什么。对 ``if`` 而言，有三个可能的阶段：

- 调用 ``call`` 之前，屏蔽 <consequence> 和 <alternative> 的求值，只计算 <condition>
- 首次调用 ``call`` ，即根据传入的已经计算完毕的 <condition> 决定是计算 <consequence> 还是 <alternative>
- 再次调用 ``call`` ，求值器已经算好了答案，将其入栈即可。整个过程结束。

为了能够让求值器按照 ``if`` 的特殊行为模式运行，我们就需要将 ``call`` 接口设计成上文所述的通用形式。继续下一阶段只需将栈顶 ``if`` 恢复，并且将PC指向下一阶段的表达式入口。而如果要结束整个 ``if`` 反复过程，则不需要恢复 ``if`` , 读出返回地址，将结果入栈并且设PC为返回地址的下一条指令。

这种设计看上去有些繁琐，但事实上证明是有效而具备弹性的。例如我在实现一系列builin-procedure时，就不需要考虑这种入栈出栈的事情，因为它们的操作都是相同的，只是计算过程不同，所以只需编写相应的函数代入 ``BuiltinProcObj`` 中即可。而实现像 ``if`` 这种特殊的算符，有了generic的接口能够轻松地操作执行流程，虽然直接改动了求值器的流程，但是影响也是局部的。

值得注意的 ``if`` 三部曲中有一个在调用 ``call`` 之前的屏蔽操作。因为类似 ``if`` 还有 ``lambda`` 的 primitive-macro 并不是按照procedure的模式工作，而是有选择性地计算参数列表中的参数。关于这些的实现，我将在下面的Interpretation中详细阐述。

Interpretation
--------------

整个解释器的核心问题就是如何求值。按照实现上可以分为两种。

一种是开发快速，可读性好的递归实现。它的优点是代码简单，思路直接。实现方式就是扫描一个表达式，将相应的语法成分抽取出来，递归计算这些部分后再根据语义得出整个表达式的结果。它的优点有：

- 设计障碍小，容易实现
- 在复杂表达的处理上较为轻松
- 直接按照BNF文法硬编码

而缺点则是：

- 受到实现语言的限制（栈大小等）
- 在Scheme递归调用足够深时，程序会因为栈空间不足而退出，难以恢复和继续运行（当然应该有机制能捕捉即将发生的栈溢出，不过的确不是特别好的方法）
- 花销较大，效率不可观。直接实现的最坏复杂度是Scheme表达式长度乘以递归深度，最坏是O(n^2)量级。

因此在构建AST，求值过程以及外部表示（external representation）的生成中，我都采用非递归实现。值得一提的是之所以 external representation 要采取非递归实现，主要原因是circular structure的判定，例如下面的代码：

::

    define x '(1 . 2))
    (set-cdr! x x)
    (display x)

可以看出x实际上是一个自我指涉的结构，不妨将其视作一个无限长的list。而采用非递归的实现能较为容易地检测这种情形：

::

    execute: build/sonsi in.scm
    output: (1 #inf#)

类似的情况还出现在

::
    
    ; Circular promise
    (define ones (delay (cons 1 ones)))
    (display (car (force ones)))
    (display "\n")
    
    ; Circular vector
    (define x #(1))
    (vector-set! x 0 x)
    (display x)
    (display "\n")
    
    ; Indirectly circular pair
    (define x '(1 . 2))
    (define y (cons 1 x))
    (define z (cons 1 y))
    (define l (cons 1 z))
    (set-car! x l)
    (display x)
    (display "\n")

采用适当实现的sonsi均能产生正确的输出：

::

    1
    #(#inf#)
    ((1 1 1 #inf#) . 2)

非递归的AST解析过程较为简单，就不再赘述。这里重点介绍一下求值过程的设计。
在 ``eval.cpp`` 中的函数 ``run_repr`` 就是解释器的主要计算流程。可以看到是由一个while-loop反复进行栈操作实现的。
事实上，求值过程的实现略微带有VM（virtual machine)的味道，也就是说，设计一套byte-code，在把目前的实现中的一些操作加以细化，就能逐步改写成一个简单的compiler。由于这是第一次尝试完成这方面的工程，所以一开始就打算设计一个比较direct的interpreter。换而言之，虽然采用类似的结构，但是许多操作并没有被拆分成更为generic的基本指令。下文进行具体的阐释。

首先为了实现如同R5RS中所说的程序和数据同质化的效果，我直接使用了Scheme中的Pair来表示AST。这个实现有许多优点。
第一就是first-class procedure的实现便利。sonsi最初的实现是将 ``opt`` 算符作为AST子树的根，而各个参数作为儿子结点。这样的确在结构上能够反映一个procedure的调用的形式。然而让人陷入困窘的是根不一定就是立即值，而可能也是一个需要通过计算得出的procedure。那么将算符和参数用这种父子关系来表示无疑增加了算符的特异性，在实现上需要各种特判来进行弥补。另一个显而易见的问题就是，用这种结构存储表示表达式不利于 ``eval`` 的实现。 因此，最终我直接使用Pair构建的s-expression来表示AST。

其次是关于指令的问题。不像compiler能够直接将操作序列化，sonsi仅仅用在表达式上的结点地址来描述当前计算到的位置。即PC寄存器里存储的是一个 ``Pair *`` 。在s-expression的每个结点存储一个 ``next`` 域来指向下一条指令，在大多数情况下 ``next == cdr`` ，而当 ``cdr`` 是empty list时， ``next`` 是 ``NULL`` ，在特殊的语法结构，例如 ``if`` 中，通过在执行时临时修改 ``next`` 指针就能跳过相应参数的计算。 的确使用这个方式能够基本描述计算进行的状态，但并不充分。在 ``if`` 执行过程中， ``if`` 本身的执行的 **阶段** 也描述了计算的状态。这一点在compiler可以把 ``if`` 拆分为更简单的指令，而在直接解释的实现中，我通过在Continuation中用 ``Pair *state`` 记录当前call的阶段。 什么是Continuation呢？

Continuation类似与栈帧，记录Scheme中caller的状态，在call计算完成后，caller通过自身所处的Continuation能恢复出call发生前的envt，cont和pc寄存器的值。

Let's put them all together.

求值器拥有三个寄存器：

- envt 环境指针
- cont 当前Continuation
- pc program counter （当前指令位置）

envt提供了从Symbol到相应值的正确映射，不同的Environment实例通过 ``prev_envt`` 指针形成层级结构，并且从底而上地查询Symbol是否bound。这样局部变量能够产生shadow的效果，并且利用指针和GC，Environment层级结构能够节省大量空间。

cont指向当前的Continuation，而每个Continuation记录了：

- prev_cont 上一层调用 （父调用）
- envt 进入调用前的envt寄存器值
- pc 进入调用前pc寄存器的值
- prog caller的AST根结点（方便 ``if`` 等特殊算符解析参数）
- tail true 表示当前call是可以尾递归优化的，并且已经执行到tail expression

求值器拥有一个求值栈 ``eval_stack`` （与Continuation串联形成的调用“栈”不同，这个是用来计算表达式的）

主循环 ``while (cont == bcont)`` ，将一直进行，除非最低层的call已经退出。

- 考察当前pc指向的AST结点

  - pc 为 ``NULL``

    - 说明对于当前call来说各个参数已经计算完毕
    - 开始调用call，具体方法就是调用 ``OptObj`` 的通用接口 ``call`` 函数
    - ``call`` 函数借由多态性完成调用

      - 根据cont寄存器得到进入调用前的相关信息

        - 还有阶段没有完成：将pc设置为下一阶段的位置，更新 ``cont->state``
        - 所有阶段完成：根据cont恢复寄存器，设置合适的pc值，将cont设置为prev_cont

  - pc 不为 ``NULL``

    - 当前pc指向的是立即值：从envt中获取，或者直接使用
    - 当前pc指向一个call：

      - 新建Continuation，保存当前执行状态(pc, envt, cont)，更新cont
      - 调用prepare：供有些特殊语法（ ``if`` , ``lambda`` 等）重新设置 ``next`` 指针实现部分参数的屏蔽

具体实现请参考 ``eval.cpp`` ， 关于用户自定义函数以及其闭包的实现请参考 ``ProcObj`` 构造函数和其成员函数 ``call`` 。

Garbage Collection
------------------

sonsi的垃圾回收采用引用计数的方式，并且能够对循环引用进行正确的处理。

GC的实现最初采取集中式的记录，即采用STL的map将 ``EvalObj`` 的地址和计数关联，并且扫描出计数为0的 ``EvalObj`` 予以释放。
首先每次进行GC时需要对整个map进行检查相当花费时间，我们没有必要这样做。
事实上，增加一个pending_list，每当有 ``EvalObj`` 计数归0时，就加入pending_list，那么GC时只需扫描这个list即可。值得注意的是，pending_list只是记录了曾经计数一度减少到0的 ``EvalObj`` ，而这不一定说明它现在一定就是零引用（虽然大多数情况是，否则这个实现就不能提高效率了）。因此，在GC时需要re-check每个list中的元素。

通过这个优化，我自己写的（比较臃肿低效的）同一段八皇后Scheme代码解释时间从7s降低到了2s。

通过gprof可以看出，程序时间花销的瓶颈出现在了频繁的attach和expose上，毕竟它们都是在map上进行的操作。

继续分析，不难发现其实不用采用集中式的记录。集中式记录相比分散的最大好处是对象本身不需要存储count，在并不是所有对象都需要GC时可以节约对象上维护GC信息的花销。然而在sonsi中，每一个 ``EvalObj`` 都需要在GC中拥有相应的条目，所以不妨直接改造 ``EvalObj`` 结构，加入一个 ``GCRecord *gc_rec`` 域，指向对应的GC信息。这样就可以把集中式的map去掉，转化为利用内存寻址完成map本来要做的事情。这样就可以在较小的常数时间内对一个 ``EvalObj`` 的count进行操作。

这样还剩下最后一个问题——循环引用的解决。在sonsi的实现中，函数闭包会直接产生一个循环引用：即一个函数 ``ProcObj`` 依赖于它被创生出的环境 envt，而它诞生的环境本身有存在一个到该 ``ProcObj`` 的binding。要保证闭包的正常工作，以及环境的一致性，就不可避免造成了引用计数上的问题。

例如在我的Scheme八皇后实现中（详见test/robust_test.scm中部分代码）为了编写方便，采用了大量的闭包，因此必须解决这个问题。

通过查阅资料，我发现了一个介绍CPython中GC实现的文章，里面谈到了这个问题。其中采用了一个非常简洁有效的方法，其实是逆向思维：

- 会出现循环引用问题的一定是那种带有Container性质的对象（例如Scheme中的pair, list, vector等）
- 扫描出当前没有被回收的Container
- 增加一个额外标记gc_refs，初始值设置为当前引用计数值
- 遍历所有的Container，将其依赖的Container计数减1
- 再次扫描所有的Container，其中计数不为零的必然不能回收，并且它们直接或者间接依赖的也不能回收，其余的则一定可以回收。

一个问题是，采用分散的存储之后如何遍历所有的 ``EvalObj`` ? 其实只需要用链表把 ``GCRecord`` 串起来即可。
另外，不难看出，解决循环引用虽说是必要的，但耗时的确高于普通的回收过程（检查pending_list，均摊之后可以认为没有太大的时间消耗），因为每次要扫描所有对象。因此普通的回收可以较为频繁的进行，这样一方面普通回收不会因此提高overhead，另一方面降低了没有回收的对象数，加快了循环检查。而循环检查则需要等对象数堆积到了一定程度之后再开始进行，避免产生极高的overhead。

还有一个问题是safe point。虽然之前听说过这个概念，但是真正意识到重要性是在系统加入了tail-recursive优化之后。就是说在执行过程中，某对象引用计数一度降为零，而后来又可能被增加。那么如果GC发生在这中间，就会错判，因此回收时机的选择尤为重要。最终的实现是在每次离开调用，即根据cont寄存器指向的Continuation恢复调用前状态后进行collect。

built-in procedures中提供了修改阈值和查看未被回收的对象数的过程，见Features。

Features
--------

除了baseline之外，sonsi还大致有哪些extra features呢？

- Non-recursive evaluation
- Relatively high-efficient evaluation
- Huge input support （例如构造一个10^5长度的表达式，guile直接段错误，而sonsi能够处理。这个可以启发一些应用：解释其他程序生成的Scheme代码，所谓其他程序可能是user-friendly的GUI等）
- GC Built-ins ( ``gc-status`` 和 ``set-gc-resolve-threshold!`` )
- Accurate GC (Tested, using ``(set-gc-resolve-threshold! 0)`` at the end of the script)
- Vector support
- Nearly full literal/quote support ( ``#(1 2 3 (4 5 (6 7)) 8 9)'', `` '(a, b, c)``, etc.)
- Extensive interface (can easily write more built-ins)
- And more...

What is Not Supported (feasible in the future)
----------------------------------------------

- Macro
- Quasi-quotation
