import pytest
from zeno.di.injector import Injector


# def test_scope_set_get_det():
#     s = Scope()
#     s["hello"] = 1
#     assert s["hello"] == 1

#     del s["hello"]

#     with pytest.raises(KeyError):
#         x = s["hello"]


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
