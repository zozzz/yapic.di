# flake8: noqa
# type: ignore
import pytest
from yapic.di import Injector

injector = Injector()


class A:
    pass


class B:
    pass


class C:
    pass


class D:
    pass


injector.provide(A)
injector.provide(B)
injector.provide(C)
injector.provide(D)


def fn_arg1(arg1: A):
    assert isinstance(arg1, A)


def fn_arg2(arg1: A, arg2: B):
    assert isinstance(arg1, A)
    assert isinstance(arg2, B)


def fn_arg3(arg1: A, arg2: B, arg3: C):
    assert isinstance(arg1, A)
    assert isinstance(arg2, B)
    assert isinstance(arg3, C)


def fn_arg4(arg1: A, arg2: B, arg3: C, arg4: D):
    assert isinstance(arg1, A)
    assert isinstance(arg2, B)
    assert isinstance(arg3, C)
    assert isinstance(arg4, D)


def fn_def_arg1(arg1: "Unprovided" = 1):
    assert arg1 == 1


def fn_def_arg2(arg1: A, arg2: "Unprovided" = 1):
    assert isinstance(arg1, A)
    assert arg2 == 1


def fn_def_arg3(arg1: A, arg2: "Unprovided" = 1, arg3: "Unprovided" = 2):
    assert isinstance(arg1, A)
    assert arg2 == 1
    assert arg3 == 2


def fn_def_arg4(arg1: C, arg2: "Unprovided" = 1, arg3: "Unprovided" = 2, arg4: "Unprovided" = 3):
    assert isinstance(arg1, C)
    assert arg2 == 1
    assert arg3 == 2
    assert arg4 == 3


def fn_def_arg5(arg1: C, arg2: "Unprovided" = 1, arg3: "Unprovided" = 2, arg5: C = 5):
    assert isinstance(arg1, C)
    assert arg2 == 1
    assert arg3 == 2
    assert isinstance(arg5, C)


def fn_kwonly1(*, kw1: A):
    assert isinstance(kw1, A)


def fn_kwonly2(*, kw1: A, kw2: B):
    assert isinstance(kw1, A)
    assert isinstance(kw2, B)


def fn_kwonly3(*, kw1: A, kw2: B, kw3: C):
    assert isinstance(kw1, A)
    assert isinstance(kw2, B)
    assert isinstance(kw3, C)


def fn_kwonly4(*, kw1: A, kw2: B, kw3: C, kw4: D):
    assert isinstance(kw1, A)
    assert isinstance(kw2, B)
    assert isinstance(kw3, C)
    assert isinstance(kw4, D)


def fn_def_kwonly1(*, kw1: "X" = 1):
    assert kw1 == 1


def fn_def_kwonly2(*, kw1: "X" = 1, kw2: "Y" = 2):
    assert kw1 == 1
    assert kw2 == 2


def fn_def_kwonly3(*, kw1: A, kw2: "X" = 2, kw3: "Y" = 3):
    assert isinstance(kw1, A)
    assert kw2 == 2
    assert kw3 == 3


def fn_def_kwonly4(*, kw1: A, kw2: "X" = 2, kw3: "X" = 3, kw4: "X" = 4):
    assert isinstance(kw1, A)
    assert kw2 == 2
    assert kw3 == 3
    assert kw4 == 4


def fn_def_kwonly5(*, kw1: A, kw2: "X" = 2, kw3: "X" = 3, kw5: C = 5):
    assert isinstance(kw1, A)
    assert kw2 == 2
    assert kw3 == 3
    assert isinstance(kw5, C)


def fn_mixed1(a1: A, *, kw1: B):
    assert isinstance(a1, A)
    assert isinstance(kw1, B)


def fn_mixed2(a1: A, a2: B, *, kw1: C):
    assert isinstance(a1, A)
    assert isinstance(a2, B)
    assert isinstance(kw1, C)


def fn_mixed3(a1: A, *, kw1: B, kw2: C):
    assert isinstance(a1, A)
    assert isinstance(kw1, B)
    assert isinstance(kw2, C)


def fn_mixed4(a1: A, a2: "X" = 2, *, kw1: B, kw2: C):
    assert isinstance(a1, A)
    assert isinstance(kw1, B)
    assert isinstance(kw2, C)
    assert a2 == 2


def fn_mixed5(a1: A, a2: "X" = 2, *, kw1: B, kw2: C, kw3: "Y" = 3):
    assert isinstance(a1, A)
    assert isinstance(kw1, B)
    assert isinstance(kw2, C)
    assert a2 == 2
    assert kw3 == 3


FNS = (fn_arg1, fn_arg2, fn_arg3, fn_arg4, fn_def_arg1, fn_def_arg2, fn_def_arg3, fn_def_arg4, fn_def_arg5, fn_kwonly1,
       fn_kwonly2, fn_kwonly3, fn_kwonly4, fn_def_kwonly1, fn_def_kwonly2, fn_def_kwonly3, fn_def_kwonly4,
       fn_def_kwonly5, fn_mixed1, fn_mixed2, fn_mixed3, fn_mixed4, fn_mixed5)


@pytest.mark.parametrize("fn", FNS)
def test_signatures(fn):
    injector.exec(fn)
