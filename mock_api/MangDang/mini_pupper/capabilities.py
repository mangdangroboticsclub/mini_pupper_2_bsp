class Capabilities:

    def __init__(self, image_dir='/var/lib/mini_pupper_bsp'):
        self.capabilities = {}
        self.capabilities['version'] = 'mp2'
        self.capabilities['imu'] = 'onboard'
        self.capabilities['microphone'] = 'onboard'
        self.capabilities['speaker'] = 'onboard'

    def has_capability(self, capability):
        return capability in self.capabilities

    def get_capability(self, capability):
        if not self.has_capability(capability):
            return None
        return self.capabilities[capability]

    def add_capability(self, capability, description):
        if self.has_capability(capability):
            return False
        self.capabilities[capability] = description
        return True

    def remove_capability(self, capability):
        if not self.has_capability(capability):
            return False
        del self.capabilities[capability]
        return True

    def list_capabilities(self):
        return self.capabilities
