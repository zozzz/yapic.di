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
    # assert isinstance(b.a, A)
