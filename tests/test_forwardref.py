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
