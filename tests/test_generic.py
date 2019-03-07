import pytest
from typing import Generic, TypeVar
from zeno.di import Injector, Inject


def test_generic_attr():
    class A:
        pass

    T = TypeVar("T")

    class B(Generic[T]):
        a: Inject[T]

    injector = Injector()
    injector.provide(A)
    injector.provide(B, B[A])

    b = injector.get(B)
    assert isinstance(b, B)
    assert isinstance(b.a, A)


def test_generic_init():
    class A:
        pass

    T = TypeVar("T")

    class B(Generic[T]):
        def __init__(self, a: T):
            assert isinstance(a, A)

    injector = Injector()
    injector.provide(A)
    injector.provide(B, B[A])

    b = injector.get(B)
    assert isinstance(b, B)


def test_generic_inherit_init():
    T = TypeVar("T")

    class Z:
        pass

    class A(Generic[T]):
        def __init__(self, a: T):
            self.a_init = a

    class B(A[T]):
        pass

    injector = Injector()
    injector.provide(A)
    injector.provide(Z)
    injector.provide(B, B[Z])

    b = injector.get(B)
    assert isinstance(b, B)
    assert isinstance(b.a_init, Z)


def test_generic_inherit_attr():
    class A:
        pass

    T = TypeVar("T")

    class B(Generic[T]):
        a: Inject[T]

    X = TypeVar("X")

    class C(B[X]):
        x: Inject[X]

    injector = Injector()
    injector.provide(A)
    injector.provide(C, C[A])

    c = injector.get(C)
    assert isinstance(c, C)
    assert isinstance(c.a, A)
    assert isinstance(c.x, A)


def test_generic_param():
    T = TypeVar("T")

    class A(Generic[T]):
        b: Inject[T]

    class B:
        pass

    def fn(a: A[B]):
        assert isinstance(a, A)
        assert isinstance(a.b, B)
        return "OK"

    injector = Injector()
    injector.provide(A)
    injector.provide(B)

    assert injector.exec(fn) == "OK"


def test_generic_param2():
    T = TypeVar("T")

    class A(Generic[T]):
        b: Inject[T]

    class B:
        pass

    class C:
        pass

    def fn(a: A[B]):
        assert isinstance(a, A)
        assert isinstance(a.b, C)
        return "OK"

    injector = Injector()
    injector.provide(A[B], A[C])
    injector.provide(C)
    injector.provide(fn)

    assert injector.get(fn) == "OK"


def test_generic_extreme():
    T = TypeVar("T")
    BT = TypeVar("BT")
    C1 = TypeVar("C1")
    C2 = TypeVar("C2")
    D1 = TypeVar("D1")
    D2 = TypeVar("D2")
    D3 = TypeVar("D3")

    class IA:
        pass

    class IB:
        pass

    class IC:
        pass

    class ID:
        pass

    class X:
        pass

    class A(Generic[T]):
        a: Inject[T]

        def __init__(self, a_init: T):
            self.a_init = a_init

    class B(A[BT]):
        b: Inject[BT]

    class C(B[C2], A[C1], Generic[C1, C2, T]):
        c: Inject[T]

    class NoGeneric:
        ng: Inject[X]

    class D(NoGeneric, Generic[D1, D2, D3, T], C[D1, D2, D3]):
        d: Inject[T]
        atom: Inject[A[D3]]

    injector = Injector()
    injector.provide(IA)
    injector.provide(IB)
    injector.provide(IC)
    injector.provide(ID)
    injector.provide(X)

    injector.provide(A, A[IA])
    injector.provide(B, B[IB])
    injector.provide(C, C[IA, IB, IC])
    injector.provide(D, D[IA, IB, IC, ID])

    a = injector.get(A)
    assert isinstance(a, A)
    assert isinstance(a.a, IA)
    assert isinstance(a.a_init, IA)

    b = injector.get(B)
    assert isinstance(b, B)
    assert isinstance(b.b, IB)
    assert isinstance(b.a, IB)
    assert isinstance(b.a_init, IB)

    c = injector.get(C)
    assert isinstance(c, C)
    assert isinstance(c.c, IC)
    assert isinstance(c.b, IB)
    assert isinstance(c.a, IA)
    assert isinstance(c.a_init, IA)

    d = injector.get(D)
    assert isinstance(d, D)
    assert isinstance(d.d, ID)
    assert isinstance(d.c, IC)
    assert isinstance(d.b, IB)
    assert isinstance(d.a, IA)
    assert isinstance(d.a_init, IA)
    assert isinstance(d.ng, X)
    assert isinstance(d.atom, A)
    assert isinstance(d.atom.a, IC)
    assert isinstance(d.atom.a_init, IC)
