from typing import Generic, TypeVar

import xxx
from zeno.di import Injector


class A:
    pass


def test_forwardref_fn():
    def fn(a: "A"):
        assert isinstance(a, A)
        return "OK"

    injector = Injector()
    injector.provide(A)
    assert injector.exec(fn) == "OK"


def test_forwardref_class():
    class B:
        a: "A"
        b: "xxx.SomeModule_A"
        c: "xxx.some_module.SomeModule_B"

    injector = Injector()
    injector.provide(A)
    injector.provide(B)
    injector.provide(xxx.SomeModule_A)
    injector.provide(xxx.some_module.SomeModule_B)
    assert isinstance(injector.get(B), B)
    assert isinstance(injector.get(B).a, A)
    assert isinstance(injector.get(B).b, xxx.SomeModule_A)
    assert isinstance(injector.get(B).c, xxx.some_module.SomeModule_B)


class X:
    pass


class Y:
    x: "X"

    def __init__(self, xxx: "X"):
        assert self.x is not xxx
        assert isinstance(self.x, X)
        assert isinstance(xxx, X)


ginjector = Injector()
ginjector.provide(X)
ginjector.provide(Y)


def test_forwardref_class_global():
    def fn(y: "Y"):
        assert isinstance(y, Y)
        assert isinstance(y.x, X)
        return "OK"

    injector = ginjector.descend()
    injector.provide(fn)

    assert injector[fn] == "OK"
