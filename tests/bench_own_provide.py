from zeno.di import Injector

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
    factory = injector.provide(fn)

    benchmark.pedantic(lambda i: factory(i), args=(injector,), iterations=ITERS, rounds=100)
