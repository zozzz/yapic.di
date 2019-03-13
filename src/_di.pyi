from typing import Any, Callable, List, Optional, Tuple, TypeVar, Union

from mypy_extensions import NamedArg, VarArg


class InjectorError(Exception):
    """ Base class for dependency injection errors"""


class ProvideError(InjectorError):
    """ Raise this error, when error occured in provide phase"""


class InjectError(InjectorError):
    """ Raise this error, when error occured in injection phase"""


class NoKwOnly(ProvideError):
    """ User raise this error, when no suitable KwOnly argument is available """


FACTORY = 1
SINGLETON = 2
SCOPED = 3
CUSTOM = 4
VALUE = 5


class BoundInjectable:
    def __call__(self) -> Any:
        """ Return provided injectable result

        example::

            injector = Injector()

            class A:
                pass

            injectable = injector.provide(A)
            bound = injectable.bind(injector)
            assert isinstance(bound, BoundInjectable)
            assert isinstance(bound(), A)
        """


class Injectable:
    def bind(self, injector: "Injector") -> BoundInjectable:
        """ Bind injectable to injector """

    # TODO: delete resolve, or KIDERÍTENI MIRE JÓ
    def resolve(self, injector: "Injector") -> Any:
        pass

    def __call__(self, injector: "Injector") -> Any:
        """ Resolve injectable value with the given injector """


Provide_id = object
Provide_value = Optional[Union[type, object, Callable]]
Provide_strategy = Optional[int]
# Provide_provide = Optional[List[Tuple[Provide_id, Provide_value, Provide_strategy, "Provide_provide"]]]
Provide_provide = Optional[List[Tuple[Provide_id, Provide_value, Provide_strategy, Any]]]

CR = TypeVar("CR")


class Injector:
    def provide(self,
                id: Provide_id,
                value: Provide_value = None,
                strategy: Provide_strategy = FACTORY,
                provide: Provide_provide = None) -> Injectable:
        pass

    def get(self, id: Provide_id) -> Any:
        """ Resolve injected value with the given id """

    def exec(self, callable: Callable[..., CR], provide: Provide_provide = None) -> CR:
        """ Executes the callable with the current injector.

        DO NOT USE IN PERFORMANCE CRITICAL SCNARIOS.

        This method is good for unit test, example::

            injector = Injector()

            @injector.provide
            class A:
                pass

            @injector.exec
            def test_something(a: A):
                assert isinstance(a, A)
        """

    def descend(self) -> "Injector":
        """ Returns a new Injector instance and inherits providers from the current Injector """

    def injectable(value: Provide_value = None,
                   strategy: Provide_strategy = FACTORY,
                   provide: Provide_provide = None) -> Injectable:
        """ Returns a new Injectable instance. Useful for caching injectable without provideing """

    def __getitem__(self, id: Provide_id) -> Any:
        """ Resolve injected value with the given id, slightly faster than ``get`` method """


class ValueResolver:
    pass


class KwOnly:
    """ Provide keyword only arguments

    example::
        injector = Injector()

        class Config(dict):
            def __init__(self):
                super().__init__(some_key="OK")

        def get_kwarg(config: Config, *, name, type):
            assert name == "some_key"
            assert type is str
            return config[name]

        def fn(*, some_key: str):
            assert some_key == "OK"
            return "NICE"

        injector.provide(Config)
        injector.provide(KwOnly(get_kwarg))

        assert injector.exec(fn) == "NICE"

    """

    def __init__(self, callable: Callable[[
            VarArg(Any),
            NamedArg(type=str, name="name"),
            NamedArg(type=Any, name="type"),
    ], Any]):
        pass
