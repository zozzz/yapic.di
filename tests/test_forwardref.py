import xxx
from yapic.di import Injector, Inject


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
        a: Inject["A"]
        b: Inject["xxx.SomeModule_A"]
        c: Inject["xxx.some_module.SomeModule_B"]

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
    x: Inject["X"]

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


def test_forwardref_class2():
    class Z:
        z: Inject[xxx.GenericClass["X"]]

    injector = Injector()
    injector.provide(X)
    injector.provide(xxx.GenericClass)
    injector.provide(Z)

    assert isinstance(injector.get(Z), Z)
    assert isinstance(injector.get(Z).z, xxx.GenericClass)
    assert isinstance(injector.get(Z).z.attr, X)
    assert isinstance(injector.get(Z).z.x_init, X)
