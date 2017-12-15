from zeno.di import Injector, VALUE, FACTORY, SCOPED_SINGLETON, SINGLETON


def test_strategy_value():
    injector = Injector()
    provided = "VALUE"
    injector.provide("V", provided, VALUE)

    assert injector["V"] == "VALUE"
    assert injector["V"] is provided


def test_strategy_custom():
    cscope = dict()

    def custom_strategy(factory):
        try:
            return cscope[factory]
        except KeyError:
            value = cscope[factory] = factory()
            return value

    class A:
        pass

    injector = Injector()
    injector.provide(A, A, custom_strategy)

    assert isinstance(injector[A], A)
    assert injector[A] is injector[A]

