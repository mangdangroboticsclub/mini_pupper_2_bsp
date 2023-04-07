from MangDang.mini_pupper.capabilities import Capabilities

mp = Capabilities()

assert(mp.has_capability('version') is True)
assert(mp.has_capability('nonsense') is False)
print(mp.get_capability('version'))
# please note that adding or removing a capability is not persistent.
# application needs to queery environment before adding or removing a capability
assert(mp.add_capability('version', 'test') is False)
assert(mp.add_capability('new_capability', 'my new capability') is True)
assert(mp.add_capability('new_capability', 'my new capability') is False)
for key, value in mp.list_capabilities().items():
    print("%s: %s" % (key, value))

assert(mp.remove_capability('new_capability') is True)
assert(mp.remove_capability('new_capability') is False)
for key, value in mp.list_capabilities().items():
    print("%s: %s" % (key, value))
