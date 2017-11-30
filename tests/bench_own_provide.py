from zeno.di import Injector


def test_native_call(benchmark):
    class A:
        pass

    def fn(a: A):
        return a

    benchmark.pedantic(lambda i: fn(i()), args=(A,), iterations=10000, rounds=100)


def test_injector_get(benchmark):
    class A:
        pass

    def fn(a: A):
        return a

    injector = Injector()
    injector.provide(A)
    injector.provide(fn)

    benchmark.pedantic(lambda i: injector.get(i), args=(fn,), iterations=10000, rounds=100)


def test_own_provide(benchmark):
    class A:
        pass

    def fn(a: A):
        return a

    injector = Injector()
    injector.provide(fn, provide=[A])

    benchmark.pedantic(lambda i: injector.get(i), args=(fn,), iterations=10000, rounds=100)
