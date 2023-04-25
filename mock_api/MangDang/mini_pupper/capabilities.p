class Capabilities:

    def __init__(self):
        self.capabilities = {}
        self.capabilities['version'] = 'mini_pupper_2'
        self.capabilities['imu'] = 'onboard'
        self.capabilities['microphone'] = 'onboard'
        self.capabilities['speaker'] = 'onboard'

    def has_capability(self, capability):
        return capability in self.capabilities

    def get_capability(self, capability):
        if not self.has_capability(capability):
            return None
        return self.capabilities[capability]

    def list_capabilities(self):
        return self.capabilities


if __name__ == '__main__':
    print(Capabilities().get_capability('version'))
