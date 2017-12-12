import pytest
from typing import Generic, TypeVar
from zeno.di import Injector


def test_generic_attr():
    class A:
        pass

    T = TypeVar("T")

    class B(Generic[T]):
        a: T

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


def test_generic_inherit_attr():
    class A:
        pass

    T = TypeVar("T")

    class B(Generic[T]):
        a: T

    X = TypeVar("X")

    class C(B[X]):
        x: X

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
        b: T

    class B:
        pass

    def fn(a: A[B]):
        assert isinstance(a, A)
        assert isinstance(a.b, B)
        return "OK"

    injector = Injector()
    injector.provide(A)
    injector.provide(B)

    assert injector.get(fn) == "OK"


def test_generic_param2():
    T = TypeVar("T")

    class A(Generic[T]):
        b: T

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

    assert injector.get(fn) == "OK"
