from MangDang.mini_pupper.capabilities import Capabilities

mp = Capabilities()

assert(mp.has_capability('version') is True)
assert(mp.has_capability('nonsense') is False)
print(mp.get_capability('version'))
for key, value in mp.list_capabilities().items():
    print("%s: %s" % (key, value))
