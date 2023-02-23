import os
import pickle

ServoCalibrationFilePath = os.path.join(os.path.dirname(__file__), 'nvmem')


def write(data):
    with open(ServoCalibrationFilePath, 'wb') as fd:
        pickle.dump(data, fd, protocol=pickle.HIGHEST_PROTOCOL)


def read():
    with open(ServoCalibrationFilePath, 'rb') as fd:
        return pickle.load(fd)


def delete():
    "this call only exist in mock_api for testing"
    os.system("rm -f ServoCalibrationFilePath")
