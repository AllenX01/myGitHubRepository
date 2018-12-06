
from functools import reduce

def main():
    print("We are in %s",__name__)
#--------------------------------------------------------------------------------
#def isPanlidrom(seq):    return seq==seq[::-1]
isPanlidrom=lambda  seq: seq==seq[::-1]
#--------------------------------------------------------------------------------
def getFactorItem(n):
    f=2
    while f*f <= n :
        #print('Begin  while loop: n=%u, f=%u' % (n,f))
        while not n%f :
            yield f
            n //= f
            #print('Middle while loop: n=%u, f=%u' % (n, f))
        f += 1
        #print('End   while loop: n=%u, f=%u' % (n, f))
    if n>1:
        #print('out of loop: n=%u, f=%u' % (n, f))
        yield n

def getFactorSeq(n):
    return [item for item in getFactorItem(n)]
#--------------------------------------------------------------------------------
def getFibItem(n):
    if(n==0):    return 0
    return 1 if n==1 else getFibItem(n-1)+getFibItem(n-2)

def getFibSeq(n):
    seq = [0]
    if n == 0: return seq
    a, b = 0, 1
    for i in range(n):
        seq.append(b)
        a, b = b, a + b
    return seq

def getFibItemG(n):
    i,a,b=0,0,1
    while i<=n:
        yield a
        a,b=b,a+b
        i +=1

def getFibSeqG(n):
    return [item for item in getFibItemG(n)]
#--------------------------------------------------------------------------------
def getFactorial(n):
    from functools import reduce
    return reduce(lambda x,y:x*y,range(1,n+1))

def getFactorial_1(n):
    a=1
    for i in range(1,n+1):
        a *=i
    return a;

def getFactorial_2(n):
    if n==0 or n==1:
        return 1
    else:
        return n*getFactorial_2(n-1)
#--------------------------------------------------------------------------------
def chgDIM_1To2(iL,step):
    oL=[iL[i:i+step] for i in range(0,len(iL),step)]
    return oL

def chgDIM_2To1(iL):
    '''
    #The below src snnipt also works fine
    oL=[]
    for i in range(len(iL)):
        for j in range(len(iL[i])):
            oL.append(iL[i][j])
    return oL
    '''
    return [iL[i][j] for i in range(len(iL)) for j in range(len(iL[i]))]
#--------------------------------------------------------------------------------
def xch2DimArrRowCol(iL):
    #return [[r[c] for r in iL] for c in range(len(iL[0]))] #also okay for working
    return list(map(list,zip(*iL))) #zip(*iL) means zip(*iL[0],iL[1],iL[2]....)
#--------------------------------------------------------------------------------
def testClassInheritance():
    '''
    mro: method resolution order
    :return:
    '''
    class Parent(object): x=1
    class Son1(Parent):   pass
    class Son2(Parent): pass

    print(Parent.x,Son1.x,Son2.x)
    Son1.x=2;    print(Parent.x,Son1.x,Son2.x)
    Son2.x=3;    print(Parent.x,Son1.x,Son2.x)
    Parent.x=4;    print(Parent.x,Son1.x,Son2.x)
    print([x.__name__ for x in Son1.mro()])
    print([x.__name__ for x in Son2.__mro__])
    print([x.__name__ for x in Parent.mro()])
#--------------------------------------------------------------------------------
def testDecorator():
    '''
    装饰器本质上是一个python函数，也是用def来定义
    它可以让其他函数在不需要做任何代码变动的前提下增加额外功能，装饰器的返回值也是一个函数对象(即：函数在内存中的地址)。
    它经常用于有切面需求的场景，比如：插入日志、性能测试、事务处理、缓存、权限验证等场景，装饰器是解决这类问题的绝佳设计，
    有了装饰器，我们就可以抽离出大量与函数功能本身无关的雷同代码并继续重用。
    概括的讲，装饰器的作用就是为已经存在的对象添加额外的功能。
    :return:
    '''
    from    functools import wraps

    def print_func_log(func):
        #@wraps(func)  #不改变使用装饰器原有函数的结构(如__name__, __doc__)
        def wrapper(*args,**kwargs):
            print('function: '+func.__name__+' start...')
            t=func(*args,**kwargs)
            print('function: '+func.__name__+'   end...')
            return t  #同样要返回原函数的返回值
        return wrapper

    @print_func_log         #@是装饰器的语法糖，它实际就是等于add=print_func_log(add)
    def add(x,y):
        print('(%s,%d,%d)'%(add.__name__,x,y))
        return (x+y)

    #add=print_func_log(add)
    t=add(2,3)
    print('add.__name=%s,return value=%d' %(add.__name__, t)) #add.__name__=wrapper 如果不使用@wraps(func)

    '''
    代码简要说明： 
    @timeit装饰器对sleep函数进行了装饰，这是一个装饰器是带参数的装饰器，
    这个装饰器由于需要传递参数，因此需要两层装饰，
    第一层是接受传递的参数，第二层是接收传递进来的需要装饰的函数当sleep函数调用的时候，调用的并不是我们看到的原始的sleep函数，而是装饰过后的sleep函数。
    这个装饰的过程会发生在调用sleep函数之前发生。
    装饰的过程：装饰器第一次接收cpu_time参数，然后返回一个dec装饰器，而这个dec装饰器会被再次调用，传入参数是原生的sleep函数，原生的sleep函数作为参数传递到dec装饰器函数的fn参数，
    通过@wraps这个装饰器将fn的属性赋值给底下需要返回的wrap函数，最后返回wrap函数，由此可间，wrap就是装饰过后的sleep函数了。
    那么在调用新sleep函数的时候，就是调用wrap函数，sleep函数的参数1，被wrap函数的*args、**kwargs这两个可变参数接收。
    整个装饰的目的就是将原生的sleep函数，扩充了一个time_func = time.clock if cpu_time else time.time和print(nowTime() - start)的过程。
    
    '''

    import  time
    def timeit(cpu_time=False):
        time_func=time.clock if cpu_time else time.time
        def dec(fn):
            #@wraps(fn)
            def wrap(*args, **kwargs):
                start=time_func()
                ret=fn(*args,**kwargs)
                print(time_func()-start)
                return ret
            return wrap
        return dec

    @timeit(False)
    def sleep(n):
        print("Now, I am start to sleep...")
        time.sleep(n)
        print("Now, I am   end to sleep...")
        return n
    #ret=sleep(3)
    #print(('ret=%u， ' % ret)+sleep.__name__)

#--------------------------------------------------------------------------------
def testClassAttrAccess():
    '''
    只想回答一个问题: 当编译器要读取obj.field时, 发生了什么?

看似简单的属性访问, 其过程还蛮曲折的. 总共有以下几个step:
1. 如果obj 本身(一个instance )有这个属性, 返回. 如果没有, 执行 step 2
2. 如果obj 的class 有这个属性, 返回. 如果没有, 执行step 3.
3. 如果在obj class 的父类有这个属性, 返回. 如果没有, 继续执行3, 直到访问完所有的父类. 如果还是没有, 执行step 4.
4. 执行obj.__getattr__方法.

Attribute与property, 都可翻译成属性. 虽然无论是在中文中还是英文中 它们的意思都几乎一样, 但仍有些许差别.
 Google了好几下, 找到了一个看起来比较靠谱的解释:
    According to Webster,
    a property is a characteristic that belongs to a thing’s essential nature and may be used to describe a type or species.
    An attribute is a modifier word that serves to limit, identify, particularize, describe, or supplement the meaning of the word it modifies.

简单来说, property是类的本质属性, 可用于定义和描述一个类别或物种； attribute则是用于详细说明它所描述的物体, 是物体的具体属性.
例如: 人都有嘴巴. 有的人嘴巴很大, 嘴巴是人的property之一, 而大嘴巴只能说是部分人的attribute.
从这个意义上讲, property是attribute的子集.

所有的数据属性(data attribute)与方法(method)都是attribute.
根据attribute的所有者, 可分为class attribute与instance attribute.
class或instance的所有attribute都存储在各自的__dict__属性中.


原文：https://blog.csdn.net/weixin_35653315/article/details/78164204
版权声明：本文为博主原创文章，转载请附上博文链接！

    :return:
    '''
    class A(object): a='a'
    class B(A):      b='b'
    class C(B):
        class_field='Attr: class field'
        def __getattr__(self, item):
            print('Method {}._getaddr__has been called.'.format(self.__class__.__name__))
            return item

    c=C()
    print(c.a)
    print(c.b)
    print(c.class_field)
    print(c.xdf)
    xxx=c.xdf
    type(xxx)
    print('len(c.c)=%d,type(xxx)=%s' % (len(c.xdf),type(xxx)))
    cc=C();dd=C()
    print(id(cc),id(dd))

#--------------------------------------------------------------------------------
#--------------------------------------------------------------------------------
#--------------------------------------------------------------------------------
#--------------------------------------------------------------------------------
#--------------------------------------------------------------------------------


if __name__=='__main__':
    main()
    #testClassInheritance()
    #testDecorator()
    testClassAttrAccess()

    '''
    n=5
    l=[[("%d * %d = %d" %(i,j,i*j)) for i in range(1,j+1)] for j in range(1,10)]
    #print("\n".join("\t".join(["%s*%s=%s" % (x, y, x * y) for y in range(1, x + 1)]) for x in range(1, 10)))

    x=[1,2,3]
    y=[4,5,6]
    z=[7,8,9]
    h=[2,3,4,5]
    xyzh=zip(x,y,z,h); print(list(xyzh))
    xyz=zip(x,y,z);
    xyz=list(map(list,xyz))
    print(xyz)

    print(xch2DimArrRowCol(xyz))


    size,step=10,4
    l = chgDIM_1To2(list(range(size)), step);     print(l)
    l = chgDIM_1To2('1234asdf!!@$0', step);     print(l)
    l='astdfa'
    l= chgDIM_2To1(l);     print(l)    
    x=5
    l = chgDIM_1To2(list(range(6)), 4); print(l)
    print(getFactorial(x), getFactorial_1(x), getFactorial_2(x))
    l = map(getFactorial, range(1, x + 1))
    print(list(l))
    str='as1sa'
    l=[1,2,3,'x',3,2,1]
    print(isPanlidrom(str))
    print(isPanlidrom(l))
    i=8
    print('Fib(%d)=%d'%(i,getFibItem(i)))
    print('R: %d:' % i,getFibSeq(i));
    print('G: %d:' % i,getFibSeqG(i));

    #print('hhh%d' % i,getFactorSeq(i))
'''