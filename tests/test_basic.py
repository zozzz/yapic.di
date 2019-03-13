import pytest
from zeno.di import Injector, ProvideError, InjectError


def test_provide_callable():
    """ Test only providable callables """
    # !!! THIS IS NOT AN EXAMPLE, THIS IS A BAD USAGE, BUT IT CAN WORK IN TEST !!!
    injector = Injector()

    @injector.provide
    def normalfn():
        pass

    with pytest.raises(ProvideError) as exc:

        @injector.provide
        def fn_with_args(a1):
            pass

    assert "Argument must have a type" in str(exc.value)

    # @injector.provide
    # def fn_with_defs1(a1, a2=2):
    #     pass

    # @injector.provide
    # def fn_with_defs2(a1, a2=2, a3=3):
    #     pass

    # @injector.provide
    # def fn_with_defs3(a1, a2=2, a3=3, a4=4):
    #     pass

    @injector.provide
    def fn_kwonly(*, kw1, kw2):
        pass

    @injector.provide
    def fn_kwonly_def1(*, kw1, kw2=2):
        pass

    @injector.provide
    def fn_kwonly_def2(*, kw1, kw2=2, kw3=3):
        pass

    @injector.provide
    def fn_kwonly_def3(*, kw1, kw2=2, kw3=3, kw4=4):
        pass

    @injector.provide
    def fn_mixed2(a1=1, *, kw1):
        pass

    @injector.provide
    def fn_mixed3(a1=1, *, kw4=4):
        pass

    class X:
        pass

    class X2:
        def __init__(self):
            pass

    class C:
        def __call__(self, a1=1):
            pass

    class CC:
        @classmethod
        def cls_method(cls, arg1: X):
            pass

        @staticmethod
        def static_method(arg1: X):
            pass

    injector.provide(X)
    injector.provide(X2)
    injector.provide(C())
    injector.provide(CC.cls_method)
    injector.provide(CC.static_method)

    with pytest.raises(TypeError) as exc:
        injector.provide(Injector.provide)
    exc.match("^Cannot get type hints from built / c-extension method")

    with pytest.raises(TypeError) as exc:
        injector.provide(injector.provide)
    exc.match("^Cannot get type hints from built / c-extension method")


def test_provide_attrs():
    class A:
        pass

    class B:
        a: A

    class X:
        pass

    class C(B):
        x: X

    injector = Injector()
    injector.provide(A)
    injector.provide(B)
    injector.provide(X)
    injector.provide(C)


def test_errors():
    assert isinstance(ProvideError(), TypeError)
    assert isinstance(InjectError(), TypeError)
