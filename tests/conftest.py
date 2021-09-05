def pytest_addoption(parser):
    parser.addoption("--executable", action="store")
    parser.addoption("--write-regtests", action="store_true")
