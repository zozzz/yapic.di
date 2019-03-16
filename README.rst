Dependency Injector
===================

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


For more info see `Python Stub file <src/_di.pyi>`_


Release Process
~~~~~~~~~~~~~~~

- change ``VERSION`` in ``setup.py``
- ``git add setup.py``
- ``git commit -m "chore(bump): VERSION"``
- ``git tag -a VERSION -m "chore(bump): VERSION"``
- ``git push && git push --tags``
