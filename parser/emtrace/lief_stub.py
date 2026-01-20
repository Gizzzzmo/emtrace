from pathlib import Path


@staticmethod
def parse(_: Path):
    return None


class COFF:
    class Binary:
        pass


class PE:
    class Binary:
        pass

    @staticmethod
    def parse(_: Path):
        return None


class MachO:
    class Binary:
        pass

    @staticmethod
    def parse(_: Path):
        return None


class ELF:
    class Binary:
        pass

    @staticmethod
    def parse(_: Path):
        return None


class OAT:
    class Binary:
        pass

    @staticmethod
    def parse(_: Path):
        return None
