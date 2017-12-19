import pytest
from zeno.di import Injector, VALUE, FACTORY, SCOPED_SINGLETON, SINGLETON
from zeno.di import InjectError


def test_strategy_value():
    injector = Injector()
    provided = "VALUE"
    injector.provide("V", provided, VALUE)

    assert injector["V"] == "VALUE"
    assert injector["V"] is provided

    with pytest.raises(InjectError) as exc:
        injector.provide("V", provided, VALUE)()
    exc.match("This injectable is not support '__call__', because is not a factory")


def test_strategy_custom():
    cscope = dict()

    def custom_strategy(injectable, injector):
        try:
            return cscope[injectable]
        except KeyError:
            value = cscope[injectable] = injectable(injector)
            return value

    class A:
        pass

    injector = Injector()
    injector.provide(A, A, custom_strategy)

    assert isinstance(injector[A], A)
    assert injector[A] is injector[A]


def test_strategy_scoped_singleton():
    class A:
        pass

    injector = Injector()
    injector.provide(A, A, SCOPED_SINGLETON)

    assert isinstance(injector[A], A)
    assert injector[A] is injector[A]
    assert injector[A] is injector[A]


def test_strategy_singleton():
    class A:
        pass

    injector = Injector()
    injector.provide(A, A, SINGLETON)

    assert isinstance(injector[A], A)
    assert injector[A] is injector[A]
    assert injector[A] is injector[A]
