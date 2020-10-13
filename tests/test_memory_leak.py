import sys
from yapic.di import Injector, Inject, VALUE

# run with `python_d setup.py test -- tests\test_memory_leak.py -x -s -vv -R :`


class SelfInject:
    injector: Inject[Injector]


def injectable_func():
    return 42


def injectable_func2(arg: SelfInject):
    return arg


def fn_def_arg1(arg1: "Unprovided" = 1):
    assert arg1 == 1


def fn_def_forward(arg1: "SelfInject"):
    return arg1


def test_self_inject():
    injector = Injector()
    injector.provide(SelfInject)
    x = injector[SelfInject]
    assert x.injector is injector


def test_value_inject():
    injector = Injector()
    injector.provide(SelfInject, 12345, VALUE)
    x = injector[SelfInject]
    assert x == 12345


def test_func_inject():
    injector = Injector()
    injector.provide(injectable_func)
    x = injector[injectable_func]
    assert x == 42


def test_func_inject2():
    injector = Injector()
    injector.provide(SelfInject)
    injector.provide(injectable_func2)
    x = injector[injectable_func2]
    assert isinstance(x, SelfInject)


def test_descend():
    injector = Injector()
    injector.provide(SelfInject)

    child = injector.descend()
    child.provide(injectable_func2)

    x = child[injectable_func2]
    assert isinstance(x, SelfInject)


def test_func_default():
    injector = Injector()
    injector.exec(fn_def_arg1)


def test_func_forward():
    injector = Injector()
    injector.provide(SelfInject)
    x = injector.exec(fn_def_forward)
    assert isinstance(x, SelfInject)
