Dependency Injector
===================

.. image:: https://img.shields.io/appveyor/ci/zozzz/yapic-di/master.svg?label=windows&style=flat-square
      :alt: AppVeyor
      :target: https://ci.appveyor.com/project/zozzz/yapic-di

.. image:: https://img.shields.io/circleci/project/github/zozzz/yapic.di/master.svg?label=linux&style=flat-square
      :alt: CircleCI
      :target: https://circleci.com/gh/zozzz/yapic.di

.. image:: https://img.shields.io/travis/com/zozzz/yapic.di/master.svg?label=sdist&style=flat-square
      :alt: Travis
      :target: https://travis-ci.com/zozzz/yapic.di

.. image:: https://img.shields.io/pypi/dm/yapic.di.svg?style=flat-square
      :alt: PyPI - Downloads
      :target: https://pypi.org/project/yapic.di/


Fast `Dependency Injection <https://en.wikipedia.org/wiki/Dependency_injection>`_
for Python, which is highly uses typing features.

Usage
~~~~~

.. code-block:: python

   from typing import Generic, TypeVar
   from yapic.di import *


   T = TypeVar("T")

   class Car(Generic[T]):
      engine: T

      def __init__(self, engine: T):
         self.engine = engine

   # simplified Car class
   class Car(Generic[T]):
      engine: Inject[T]

   class Gasoline:
      pass

   class Diesel:
      pass

   class Electronic:
      pass

   class DieselCar(Car[Diesel]):
      pass

   ELECTRONIC_CAR = "ELECTRONIC_CAR"  # this is not the best idea, but can work

   injector = Injector()
   injector.provide(Gasoline)
   injector.provide(Diesel)
   injector.provide(Electronic)
   injector.provide(DieselCar)
   injector.provide(ELECTRONIC_CAR, Car[Electronic])

   diesel = injector[DieselCar]
   assert isinstance(diesel, DieselCar)
   assert isinstance(diesel.engine, Diesel)

   electronic = injector[ELECTRONIC_CAR]
   assert isinstance(electronic, Car)
   assert isinstance(electronic.engine, Electronic)

   def drive_diesel(car: DieselCar):
      assert isinstance(car, DieselCar)
      assert isinstance(car.engine, Diesel)

   injector.exec(drive_diesel)


Keyword Only Arguments
----------------------

.. code-block:: python

   class Config(dict):
      def __init__(self):
         super().__init__(some_key=42)

   def get_kwarg(config: Config, *, name, type):
         if name == "some_key":
            return config[name]
         else:
            raise NoKwOnly()

    def fn(*, some_key: str):
        assert some_key == 42

   injector = Injector()
   injector.provide(Config)
   injector.provide(fn, provide=[KwOnly(get_kwarg)])


For more info see `Python Stub file <src/_di.pyi>`_ or `test files <tests>`_


Release Process
~~~~~~~~~~~~~~~

- change ``VERSION`` in ``setup.py``
- ``git add setup.py``
- ``git commit -m "chore(bump): VERSION"``
- ``git tag -a VERSION -m "chore(bump): VERSION"``
- ``git push && git push --tags``
