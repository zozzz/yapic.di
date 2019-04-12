import pytest
from yapic.di import Injector, VALUE, KwOnly, NoKwOnly, Inject, Injectable


def test_injector_descend():
    injector = Injector()
    injector.provide("X", "XValue", VALUE)
    subi = injector.descend()
    assert subi.get("X") == "XValue"


def test_custom_provide():
    injector = Injector()

    class A:
        pass

    class X(A):
        pass

    def fn(a: A):
        assert isinstance(a, X)
        return "OK"

    injector.provide(A)
    injector.provide(fn, provide=[(A, X)])

    assert injector.get(fn) == "OK"


def test_injector_descend2():
    class A:
        pass

    def fn(i: Injector, a: A):
        assert isinstance(a, A)
        assert a is ainst
        assert injector[A] is not a
        return i.exec(fn2)

    def fn2(i: Injector, a: A):
        assert isinstance(a, A)
        assert a is ainst
        assert i[A] is a
        return "OK"

    injector = Injector()
    injector.provide(A)

    iclone = injector.descend()
    ainst = iclone[A] = iclone[A]

    assert iclone.exec(fn) == "OK"

    iclone2 = iclone.descend()
    assert iclone2.exec(fn) == "OK"


def test_injector_clone():
    class A:
        pass

    def fn(i: Injector, kwarg: A):
        assert isinstance(kwarg, A)
        assert kwarg is ainst
        assert i[A] is kwarg
        assert i is not injector
        i[A] = "another"
        return injector.exec(fn2)

    def fn2(i: Injector):
        assert i[A] is ainst
        return "OK"

    injector = Injector()
    injector.provide(A)

    ainst = injector[A] = injector[A]

    def get_kw(a: A, *, name: str, type: type):
        if name == "kwarg":
            return a
        else:
            raise NoKwOnly()

    injectable = injector.provide(fn, provide=[KwOnly(get_kw)])
    assert injectable(injector) == "OK"
    assert injector[fn] == "OK"


def test_attr():
    class Request:
        injector: Inject[Injector]

        def verify(self):
            assert self.injector[Request] is self

    injector = Injector()
    injector.provide(Request)

    req_injector = injector.descend()
    req_injector[Request] = req_injector[Request]

    assert req_injector[Request] is req_injector[Request]

    def handle(req: Request):
        assert isinstance(req, Request)
        req.verify()

    handler = Injectable(handle)
    handler(req_injector)
