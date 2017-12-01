from zeno.di import Injector


def test_forwardref_basic():
    injector = Injector()

    def fn(a: "A"):
        assert isinstance(a, A)
        return "OK"

    class A:
        pass

    injector.provide(A)
    assert injector.exec(fn) == "OK"
