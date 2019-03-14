import pytest
from yapic.di import Injector, VALUE


def test_injector_descend():
    injector = Injector()
    injector.provide("X", "XValue", VALUE)
    subi = injector.descend()
    assert subi.get("X") == "XValue"


def test_custom_provide():
    injector = Injector()

    class A:
        pass

    class X(A):
        pass

    def fn(a: A):
        assert isinstance(a, X)
        return "OK"

    injector.provide(A)
    injector.provide(fn, provide=[
        (A, X)
    ])

    assert injector.get(fn) == "OK"



# def test_scope_inheritance():
#     s1 = Scope()
#     s1["level1"] = 1
#     s1["top"] = "top"

#     s2 = s1.new_scope()
#     s2["level2"] = 2

#     s3 = s2.new_scope()
#     s3["level3"] = 3

#     assert s1["top"] == "top"
#     assert s2["top"] == "top"
#     assert s3["top"] == "top"

#     assert s2["level2"] == 2
#     assert s3["level2"] == 2

#     assert s3["level3"] == 3

#     with pytest.raises(KeyError):
#         x = s1["level2"]

#     with pytest.raises(KeyError):
#         x = s1["level3"]

#     with pytest.raises(KeyError):
#         x = s2["level3"]
