
.. code-block:: python

	injector = di.Injector()
	injector.provide(ClassName, ProvidedClassName, strategy=Singleton(), provide=[
		[XYZ, ProvidedXyz],
		dict(id=XYZ, value=ProvidedXyz, provide=[...])
	])

	injector.get(XYZ)

.. code-block:: python

	class Request:
		session: Inject[Session]

		@inject
		def __init__(self, db: Database):
			pass


	class Database:
		def __init__(self, host, port):
			pass

	def config(ns: str) -> KwArgs:
		def provider(config: Config, key: str) -> Any:
			return Config.ns(ns).get(key)
		return KwArgs(provider)

	injector.provide(Database, Singleton(), provide=[
		config("db")
	])

Strategies
----------

Singleton
	Singleton...

Value
	Dont do anything with provided value just retunt it.

