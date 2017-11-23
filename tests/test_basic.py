from zeno.di import Injector


def test_xxx():
    def fn(a, b, c, *, kwonly, kw2=2):
        pass

    # test cases: 1-3 defaults
    class X:
        def __call__(self, fasz, v=1, y=2):
            pass

    class C:
        def __init__(self):
            pass

    injector = Injector()
    print(injector)
    injector.provide(fn)
    injector.provide(X)
    injector.provide(X())
    injector.provide(C)
    # injector.provide(Injector.provide) # WRONG
    # injector.provide(injector.provide) # WRONG

    assert 1 == 0
