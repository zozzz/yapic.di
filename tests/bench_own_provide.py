import threading
from zeno.di import Injector, SCOPED_SINGLETON, SINGLETON

ITERS = 20000


def test_native_call(benchmark):
    class A:
        pass

    def fn(a: A):
        return a

    benchmark.pedantic(lambda i: fn(i()), args=(A,), iterations=ITERS, rounds=100)


def test_similar_native_call(benchmark):
    class A:
        pass

    def fn(a: A):
        return a

    scope = {
        A: A,
        fn: fn
    }

    def do():
        return scope[fn](scope[A]())

    benchmark.pedantic(lambda: do(), iterations=ITERS, rounds=100)


def test_injector_get(benchmark):
    class A:
        pass

    def fn(a: A):
        return a

    injector = Injector()
    injector.provide(A)
    injector.provide(fn)

    benchmark.pedantic(lambda i: injector.get(i), args=(fn,), iterations=ITERS, rounds=100)


def test_injector_getitem(benchmark):
    class A:
        pass

    def fn(a: A):
        return a

    injector = Injector()
    injector.provide(A)
    injector.provide(fn)

    benchmark.pedantic(lambda i: injector[i], args=(fn,), iterations=ITERS, rounds=100)


def test_own_provide(benchmark):
    class A:
        pass

    def fn(a: A):
        return a

    injector = Injector()
    injector.provide(fn, provide=[A])

    benchmark.pedantic(lambda i: injector.get(i), args=(fn,), iterations=ITERS, rounds=100)


def test_cached_provide(benchmark):
    class A:
        pass

    def fn(a: A):
        return a

    injector = Injector()
    injector.provide(A)
    factory = injector.provide(fn).resolve

    benchmark.pedantic(lambda i: factory(i), args=(injector,), iterations=ITERS, rounds=100)


def test_custom_factory(benchmark):
    def cstrategy(injectable, injector):
        return injectable(injector)

    class A:
        pass

    def fn(a: A):
        return a

    injector = Injector()
    injector.provide(A, A, cstrategy)
    injector.provide(fn)

    benchmark.pedantic(lambda i: injector[i], args=(fn,), iterations=ITERS, rounds=100)


def test_custom_factory_single(benchmark):
    cscope = dict()

    def cstrategy(injectable, injector):
        try:
            return cscope[injectable]
        except:
            value = cscope[injectable] = injectable(injector)
            return value

    class A:
        pass

    def fn(a: A):
        return a

    injector = Injector()
    injector.provide(A, A, cstrategy)
    injector.provide(fn)

    benchmark.pedantic(lambda i: injector[i], args=(fn,), iterations=ITERS, rounds=100)


def test_scoped_singleton(benchmark):
    class A:
        pass

    def fn(a: A):
        return a

    injector = Injector()
    injector.provide(A, A, SCOPED_SINGLETON)
    injector.provide(fn)

    benchmark.pedantic(lambda i: injector[i], args=(fn,), iterations=ITERS, rounds=100)


def test_singleton(benchmark):
    class A:
        pass

    def fn(a: A):
        return a

    injector = Injector()
    injector.provide(A, A, SINGLETON)
    injector.provide(fn)

    benchmark.pedantic(lambda i: injector[i], args=(fn,), iterations=ITERS, rounds=100)


def test_threadlocal(benchmark):
    tl = threading.local()

    class A:
        pass

    def fn(factory):
        try:
            return tl.key
        except AttributeError:
            value = factory()
            tl.key = value
            return value

    benchmark.pedantic(lambda i: fn(i), args=(A,), iterations=ITERS, rounds=100)

    assert fn(A) is fn(A)
