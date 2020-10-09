# vim: set fileencoding=utf-8 :
import pytest

import pyvips
from helpers import VIPS_SPARC, VIPS_INTEL


class TestEndianness:

    def test_endianness(self):
        # Little-endian
        intel = pyvips.Image.new_from_file(VIPS_INTEL)
        assert intel.width == 16
        assert intel.height == 16
        assert intel.bands == 1
        assert intel.avg() == 128

        # Big-endian
        sparc = pyvips.Image.new_from_file(VIPS_SPARC)
        assert sparc.width == 16
        assert sparc.height == 16
        assert sparc.bands == 1
        assert sparc.avg() == 128


if __name__ == '__main__':
    pytest.main()
