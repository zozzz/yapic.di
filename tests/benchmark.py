# from zeno.di.injector import Injector
# from zeno.di.injector_new import Injector
from zeno.di import Injector, VALUE


# def test_scope_get(benchmark):
#     s = Scope()
#     s["hello"] = 1
#     benchmark(lambda s: s["hello"], s)


# def test_scope_inherit_get(benchmark):
#     s = Scope()
#     s["hello"] = 1

#     s2 = s.new_scope()
#     s2["xy"] = 2
#     benchmark(lambda s: s["hello"], s)


# def test_scope_new(benchmark):
#     benchmark(lambda: Scope())


def test_native_value_get(benchmark):
    value = "PROVIDED"
    benchmark.pedantic(lambda i: i, args=(value,), iterations=10000, rounds=100)


def test_native_value_get_dict(benchmark):
    x = dict(PROVIDED="PROVIDED")
    benchmark.pedantic(lambda x: x["PROVIDED"], args=(x,), iterations=10000, rounds=100)


def test_native_fn_call(benchmark):
    def fn():
        return "PROVIDED"
    benchmark.pedantic(lambda fn: fn(), args=(fn,), iterations=10000, rounds=100)


def test_injector_get(benchmark):
    i = Injector()
    i.provide("PROVIDED", "PROVIDED", VALUE)

    benchmark.pedantic(lambda i: i.get("PROVIDED"), args=(i,), iterations=10000, rounds=100)

    assert i.get("PROVIDED") == "PROVIDED"


def test_injector_getitem(benchmark):
    i = Injector()
    i.provide("PROVIDED", "PROVIDED", VALUE)

    benchmark.pedantic(lambda i: i["PROVIDED"], args=(i,), iterations=10000, rounds=100)

    assert i["PROVIDED"] == "PROVIDED"


def test_injector_injectable(benchmark):
    i = Injector()
    injectable = i.provide("PROVIDED", "PROVIDED", VALUE)

    benchmark.pedantic(lambda i: injectable(i), args=(i,), iterations=10000, rounds=100)

    assert injectable(i) == "PROVIDED"
