import pathlib
import subprocess
import tempfile

import numpy as np
import PIL
import PIL.Image
import PIL.ImageDraw
import pytest
import soundfile


@pytest.fixture
def canvas(pytestconfig):
    result = pytestconfig.getoption("executable")
    if result is None:
        raise RuntimeError(
            "Missing --executable pytest option. Please point it to the canvas executable."
        )
    return result

@pytest.fixture
def example_image():
    image = PIL.Image.new("RGB", (100, 100))
    draw = PIL.ImageDraw.Draw(image)
    draw.rectangle([(0, 0), (100, 100)], fill=(255, 0, 255))
    return image

@pytest.fixture
def example_sound():
    sample_rate = 48000
    duration = 1.0
    frequency = 440.0
    duration_in_samples = int(sample_rate * duration)
    t = np.arange(duration_in_samples) / sample_rate
    sound = np.sin(2 * np.pi * t * frequency)[:, np.newaxis]
    return sound

def test_help(canvas):
    """canvas --help runs without error."""
    subprocess.run([canvas, "--help"], check=True)

def test_image_to_image(canvas, example_image):
    """canvas with image import and export preserves color."""
    with tempfile.TemporaryDirectory() as directory:
        root = pathlib.Path(directory)
        example_image.save(root / "in.png")
        subprocess.run([canvas, "-t", "-i", root / "in.png", "-o", root / "out.png"])
        out_image = PIL.Image.open(root / "out.png")
        assert out_image.getpixel((0, 0)) == (255, 0, 255, 255)
        assert out_image.size[1] == 239

def test_image_to_sound(canvas, example_image):
    """Converting an image to sound produces non-silent audio."""
    with tempfile.TemporaryDirectory() as directory:
        root = pathlib.Path(directory)
        example_image.save(root / "in.png")
        subprocess.run([canvas, "-t", "-i", root / "in.png", "-o", root / "out.wav"])
        out_sound, __ = soundfile.read(root / "out.wav")
        assert np.any(out_sound != 0)

def test_sound_to_image(canvas, example_sound):
    """Converting a sound to image produces a non-blank image."""
    with tempfile.TemporaryDirectory() as directory:
        root = pathlib.Path(directory)
        soundfile.write(root / "in.wav", example_sound, 48000)
        subprocess.run([canvas, "-t", "-i", root / "in.wav", "-o", root / "out.png"])
        out_image = PIL.Image.open(root / "out.png")
        assert np.any(np.asarray(out_image)[:, :, :3] != 0)

def test_sound_to_sound(canvas, example_sound):
    """Converting sound to sound produces non-silent audio."""
    with tempfile.TemporaryDirectory() as directory:
        root = pathlib.Path(directory)
        soundfile.write(root / "in.wav", example_sound, 48000)
        subprocess.run([canvas, "-t", "-i", root / "in.wav", "-o", root / "out.wav"])
        out_sound, rate = soundfile.read(root / "out.wav")
        assert np.any(out_sound != 0)
