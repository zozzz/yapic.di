import pytest
from zeno.di import Injector, BoundInjectable, KwOnly


@pytest.mark.skip("Onyl for __repr__ test")
def test_injectable_call1():
    injector = Injector()

    def kw(x: "Y", *, name, type):
        pass

    class C:
        pass

    class B:
        pass

    class A:
        c: C
        b: B

        def __init__(self, c: C, d=1, *, kwonly=None):
            pass

    injectable = injector.provide(A, provide=[
        KwOnly(kw),
        C,
        B
    ])
    print(injectable)
    exit(0)
    assert isinstance(injectable(injector), A)


def test_injectable_call():
    injector = Injector()

    class A:
        pass

    injectable = injector.provide(A)
    assert isinstance(injectable(injector), A)


def test_injectable_call_err():
    injector = Injector()

    class A:
        pass

    injectable = injector.provide(A)
    with pytest.raises(TypeError) as exc:
        injectable()
    exc.match("__call__ expected 1 arguments, got 0")

    with pytest.raises(TypeError) as exc:
        injectable(None)
    exc.match("Bad argument, must call with 'Injector' instance")


def test_injectable_bind():
    injector = Injector()

    class A:
        pass

    injectable = injector.provide(A)
    bound = injectable.bind(injector)
    assert isinstance(bound, BoundInjectable)
    assert isinstance(bound(), A)


def test_injectable_bind_err():
    injector = Injector()

    class A:
        pass

    injectable = injector.provide(A)
    with pytest.raises(TypeError) as exc:
        injectable.bind(None)
    exc.match("Bad argument, must call with 'Injector' instance")

    bound = injectable.bind(injector)
    with pytest.raises(TypeError) as exc:
        bound(None)
    exc.match("__call__ expected 0 arguments, got 1")

