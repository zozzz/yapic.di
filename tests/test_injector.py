import sys
import pytest
from yapic.di import Injector, VALUE, KwOnly, InjectError, ProvideError, Inject, Token


def test_injector_value():
    injector = Injector()
    injector.provide("ID", "PROVIDED", VALUE)
    assert injector.get("ID") == "PROVIDED"


def test_injector_exec_fn():
    injector = Injector()

    class A:
        pass

    def x(a: A):
        assert isinstance(a, A)
        return "OK"

    injector.provide(A)
    assert injector.exec(x) == "OK"


def test_injector_exec_cls():
    injector = Injector()

    class A:
        pass

    class B:
        def __init__(self, a: A):
            assert isinstance(a, A)

    injector.provide(A)
    assert isinstance(injector.exec(B), B)


def test_injector_exec_object():
    BINST = "BInstance"

    injector = Injector()

    class A:
        pass

    class B:
        def __call__(self, a: A):
            assert isinstance(a, A)
            return "BBBB"

    def fn1(arg: B):
        assert isinstance(arg, B)
        return "fn1"

    def fn2(arg: BINST):
        assert arg == "BBBB"
        return "fn2"

    injector.provide(A)
    injector.provide(B)
    injector.provide(BINST, B())

    assert injector.exec(fn1) == "fn1"
    assert injector.exec(fn2) == "fn2"


def test_injector_attribute():
    injector = Injector()

    class A:
        pass

    class B:
        a: Inject[A]

        def __init__(self):
            assert isinstance(self.a, A)

    injector.provide(A)

    b = injector.exec(B)
    assert isinstance(b, B)
    assert isinstance(b.a, A)


def test_injector_attribute_inheritance():
    injector = Injector()

    class A:
        pass

    class B:
        a: Inject[A]

        def __init__(self):
            assert isinstance(self.a, A)

    class X:
        pass

    class C(B):
        x: Inject[X]

        def __init__(self):
            super().__init__()
            assert isinstance(self.x, X)

    injector.provide(A)
    injector.provide(X)

    c = injector.exec(C)
    assert isinstance(c, C)
    assert isinstance(c.a, A)
    assert isinstance(c.x, X)


def test_injector_attribute_slots():
    injector = Injector()

    class A:
        pass

    class B:
        __slots__ = ("a", )
        a: Inject[A]

        def __init__(self):
            assert isinstance(self.a, A)

    injector.provide(A)

    b = injector.exec(B)
    assert isinstance(b, B)
    assert isinstance(b.a, A)


def test_injector_kwonly():
    injector = Injector()

    class Config(dict):
        def __init__(self):
            super().__init__(some_key="OK")

    def get_kwarg(config: Config, *, name, type):
        assert name == "some_key"
        assert type is str
        return config[name]

    def fn(*, some_key: str):
        assert some_key == "OK"
        return "NICE"

    injector.provide(Config)
    injector.provide(KwOnly(get_kwarg))

    assert injector.exec(fn) == "NICE"


def test_injector_kwonly2():
    injector = Injector()

    class Config(dict):
        def __init__(self):
            super().__init__(some_key="OK")

        def __call__(self, *, name, type):
            assert name == "some_key"
            assert type is str
            return self[name]

    def fn(*, some_key: str):
        assert some_key == "OK"
        return "NICE"

    injector.provide(KwOnly(Config()))

    assert injector.exec(fn) == "NICE"


def test_injector_own_kwonly():
    injector = Injector()

    class Config(dict):
        def __init__(self):
            super().__init__(some_key="OK")

    def get_kwarg(config: Config, *, name, type):
        assert name == "some_key"
        assert type is str
        return config[name]

    def fn(*, some_key: str):
        assert some_key == "OK"
        return "NICE"

    injector.provide(Config)
    injector.provide(fn, provide=[KwOnly(get_kwarg)])

    assert injector.get(fn) == "NICE"

    with pytest.raises(InjectError) as exc:
        assert injector.exec(fn) == "NICE"
    exc.match("Not found suitable value for: <ValueResolver name=some_key id=<class 'str'>>")


def test_injector_inject_self():
    injector = Injector()

    class A:
        inj: Inject[Injector]

        def __init__(self):
            assert isinstance(self.inj, Injector)
            assert isinstance(self.inj.get(B), B)

    class B:
        pass

    injector.provide(A)
    injector.provide(B)

    assert isinstance(injector.get(A), A)
    assert isinstance(injector[A], A)


def test_injector_inject_self2():
    injector = Injector()

    class A:
        inj: Inject[Injector]

        def __init__(self):
            assert isinstance(self.inj, Injector)
            assert isinstance(self.inj.get(B), B)
            assert isinstance(self.inj.get(C), C)
            assert isinstance(self.inj.get(D), D)
            assert isinstance(self.inj.get(D).c, C)
            assert isinstance(self.inj[D].c, C)

    class B:
        pass

    class C:
        pass

    class D:
        c: Inject[C]

    injector.provide(A, provide=[C])
    injector.provide(B)
    injector.provide(D)

    assert isinstance(injector.get(A), A)

    with pytest.raises(InjectError) as exc:
        injector.get(C)
    exc.match("Not found suitable value for: <class 'test_injector.test_injector_inject_self2.<locals>.C'>")

    with pytest.raises(InjectError) as exc:
        injector.get(D)
    exc.match(
        "Not found suitable value for: <ValueResolver name=c id=<class 'test_injector.test_injector_inject_self2.<locals>.C'>>"
    )


def test_injector_kwonly_error():
    injector = Injector()

    class Config(dict):
        def __init__(self):
            super().__init__(some_key="OK")

    def get_kwarg(config: Config, *, name, type, xx):
        pass

    def fn(*, some_key: str):
        pass

    injector.provide(Config)
    injector.provide(KwOnly(get_kwarg))

    with pytest.raises(InjectError) as exc:
        injector.exec(fn)
    exc.match("^Not found suitable value for")


def test_injector_kwonly_def_error():
    injector = Injector()

    def get_kwargs1():
        pass

    def get_kwargs2(*, xyz):
        pass

    def get_kwargs3(*, type):
        pass

    with pytest.raises(ProvideError) as exc:
        injector.provide(KwOnly(get_kwargs1))
    exc.match("Callable must have kwonly arguments")

    with pytest.raises(ProvideError) as exc:
        injector.provide(KwOnly(get_kwargs2))
    exc.match("Keyword argument resolver function muts have 'name' keyword only argument")

    with pytest.raises(ProvideError) as exc:
        injector.provide(KwOnly(get_kwargs3))
    exc.match("Keyword argument resolver function muts have 'name' keyword only argument")

    class X:
        pass

    with pytest.raises(ProvideError) as exc:
        injector.provide(KwOnly(X))
    exc.match("Argument must be callable.")


def test_injector_recursion():
    injector = Injector()

    class A:
        def __init__(self, a: "A") -> None:
            pass

    injector.provide(A)
    injector.provide("A", A)

    def x(a: A):
        return "OK"

    with pytest.raises(RecursionError):
        assert injector.exec(x) == "OK"


def test_injector_setitem():
    class A:
        pass

    injector = Injector()
    injector[A] = A()

    def fn(a: A):
        assert isinstance(a, A)
        return "OK"

    assert injector.exec(fn) == "OK"

    del injector[A]

    with pytest.raises(InjectError) as exc:
        injector.exec(fn) == "OK"

    exc.match("Not found suitable value for")


def test_token():
    TOKEN = Token("__token__")

    assert repr(TOKEN) == "<Token __token__>"

    class A:
        pass

    def fn(a: TOKEN):
        assert isinstance(a, A)
        return "OK"

    injector = Injector()
    injector.provide(TOKEN, A)

    assert injector.exec(fn) == "OK"

    with pytest.raises(TypeError) as exc:
        Token(None)
    exc.match("argument 1 must be str")
