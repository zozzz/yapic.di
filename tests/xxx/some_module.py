from typing import TypeVar, Generic
from zeno.di import Inject

__all__ = ["SomeModule_A", "GenericClass"]


class SomeModule_A:
    pass


class SomeModule_B:
    pass


T = TypeVar("T")


class GenericClass(Generic[T]):
    attr: Inject[T]

    def __init__(self, x: T):
        self.x_init = x
