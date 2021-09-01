import pathlib
import subprocess
import tempfile

import PIL
import PIL.Image
import PIL.ImageDraw
import pytest


@pytest.fixture()
def canvas(pytestconfig):
    return pytestconfig.getoption("executable")


def test_help(canvas):
    """canvas --help runs without error."""
    subprocess.run([canvas, "--help"], check=True)

def test_image_import_and_export(canvas):
    """canvas with image import and export preserves color."""
    with tempfile.TemporaryDirectory() as directory:
        root = pathlib.Path(directory)
        in_image = PIL.Image.new("RGB", (100, 100))
        draw = PIL.ImageDraw.Draw(in_image)
        draw.rectangle([(0, 0), (100, 100)], fill=(255, 0, 255))
        in_image.save(root / "in.png")
        subprocess.run([canvas, "-t", "-i", root / "in.png", "-o", root / "out.png"])
        out_image = PIL.Image.open(root / "out.png")
        assert out_image.getpixel((0, 0)) == (255, 0, 255, 255)
        assert out_image.size[1] == 239

