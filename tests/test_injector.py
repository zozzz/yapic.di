import pytest
from zeno.di.injector import Injector, Provider, Factory, RecursionError, KwArgs


def test_injector_get():
    injector = Injector()
    injector.provide("PROVIDED")
    assert injector.get("PROVIDED") == "PROVIDED"


def test_injector_invoke_fn():
    injector = Injector()

    class A:
        pass

    def x(a: A):
        assert isinstance(a, A)
        return "OK"

    injector.provide(A, Factory())
    assert injector.invoke(x) == "OK"


def test_injector_invoke_class():
    injector = Injector()

    class B:
        pass

    class A:
        def __init__(self, b: B) -> None:
            assert isinstance(b, B)

    injector.provide(B, Factory())
    assert isinstance(injector.invoke(A), A)


def test_injector_recursion():
    injector = Injector()

    class A:
        def __init__(self, a: "A") -> None:
            pass

    injector.provide(A, Factory())
    injector.provide("A", A, Factory())

    def x(a: A):
        return "OK"

    with pytest.raises(RecursionError):
        assert injector.invoke(x) == "OK"


def test_injector_attribute():
    injector = Injector()

    class A:
        pass

    class B:
        a_inst: A

        def __init__(self):
            assert isinstance(self.a_inst, A)

    injector.provide(A, Factory())

    b_inst = injector.invoke(B)
    assert isinstance(b_inst, B)
    assert b_inst.a_inst is b_inst.a_inst


def test_injector_attribute2():
    injector = Injector()

    class A:
        pass

    class B:
        a_inst: A

        def __init__(self):
            assert isinstance(self.a_inst, A)

    class C:
        b_inst: B

        def __init__(self):
            assert isinstance(self.b_inst, B)
            assert isinstance(self.b_inst.a_inst, A)

    injector.provide(A, Factory())
    injector.provide(B, Factory())

    b_inst = injector.invoke(B)
    assert isinstance(b_inst, B)
    assert b_inst.a_inst is b_inst.a_inst

    c_inst = injector.invoke(C)
    assert isinstance(c_inst, C)
    assert c_inst.b_inst is c_inst.b_inst


def test_injector_inherited_attribute():
    injector = Injector()

    class A:
        pass

    class B:
        a_inst: A

    class C(B):
        x_inst: A

        def __init__(self):
            assert isinstance(self.a_inst, A)
            assert isinstance(self.x_inst, A)
            assert self.x_inst is not self.a_inst

    injector.provide(A, Factory())
    c_inst = injector.invoke(C)
    assert c_inst.a_inst is c_inst.a_inst


def test_injector_inherited_attribute2():
    injector = Injector()

    class A:
        pass

    class B:
        a_inst: A

    class X:
        pass

    class C(B):
        a_inst: X

        def __init__(self):
            assert isinstance(self.a_inst, X)

    injector.provide(A, Factory())
    injector.provide(X, Factory())

    c_inst = injector.invoke(C)
    assert c_inst.a_inst is c_inst.a_inst


def test_injector_kwargs():
    injector = Injector()

    class Config(dict):
        def __init__(self):
            super().__init__(some_key="OK")

    def get_kwarg(config: Config, keyword: str):
        return config[keyword]

    def conf_required(some_key: str, undefined: str):
        assert some_key == "OK"
        return "NICE"

    injector.provide(Config, Factory())
    injector.provide(conf_required, Factory(), provide=[
        KwArgs(get_kwarg)
    ])

    assert injector.get(conf_required) == "NICE"
