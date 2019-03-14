from typing import Generic, TypeVar, overload, NoReturn

Inj = TypeVar("Inj")
Inst = TypeVar("Inst")


class Inject(Generic[Inj]):
    """Inject token in class level

    Example::
        from yapic.di import Injector, Inject

        class Database:
            pass

        class A:
            db: Inject[Database]

        injector = Injector()
        injector.provide(Database)
        injector.provide(A)

        a_inst = injector.get(A)
        assert(isinstance(a_inst.db, Database))
    """

    @overload
    def __get__(self, instance: None, owner: type) -> NoReturn:
        pass

    @overload  # noqa
    def __get__(self, instance: Inst, owner: type) -> Inj:
        pass

    def __get__(self, instance, owner):  # noqa
        return None
