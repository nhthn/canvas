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
def stereo_image():
    image = PIL.Image.new("RGB", (100, 100))
    draw = PIL.ImageDraw.Draw(image)
    draw.rectangle([(0, 0), (100, 100)], fill=(255, 0, 128))
    return image

def sine_wave(sample_rate, duration, frequency):
    duration_in_samples = int(sample_rate * duration)
    t = np.arange(duration_in_samples) / sample_rate
    sound = np.sin(2 * np.pi * t * frequency)[:, np.newaxis]
    return sound

@pytest.fixture
def mono_sound():
    return sine_wave(48000, 1.0, 440)

@pytest.fixture
def stereo_sound():
    return np.hstack([
        sine_wave(48000, 1.0, 440),
        sine_wave(48000, 1.0, 441),
    ])

def test_help(canvas):
    """canvas --help runs without error."""
    subprocess.run([canvas, "--help"], check=True)

def test_image_to_image(canvas, stereo_image):
    """canvas with image import and export preserves color."""
    with tempfile.TemporaryDirectory() as directory:
        root = pathlib.Path(directory)
        stereo_image.save(root / "in.png")
        subprocess.run([canvas, "-t", "-i", root / "in.png", "-o", root / "out.png"], check=True)
        out_image = PIL.Image.open(root / "out.png")
        assert out_image.getpixel((0, 0))[:3] == stereo_image.getpixel((0, 0))[:3]

def test_image_to_sound(canvas, stereo_image):
    """Converting an image to sound produces non-silent, stereo audio with different
    channels."""
    with tempfile.TemporaryDirectory() as directory:
        root = pathlib.Path(directory)
        stereo_image.save(root / "in.png")
        subprocess.run([canvas, "-t", "-i", root / "in.png", "-o", root / "out.wav"], check=True)
        out_sound, __ = soundfile.read(root / "out.wav")

        assert out_sound.shape[1] == 2
        assert np.any(out_sound != 0)
        assert np.any(out_sound[:, 0] != out_sound[:, 1])

def test_mono_sound_to_image(canvas, mono_sound):
    """Converting a mono sound to image produces a non-blank image with the following
    properties:
    - blue channel equals red channel
    - green channel is blank
    """
    with tempfile.TemporaryDirectory() as directory:
        root = pathlib.Path(directory)
        soundfile.write(root / "in.wav", mono_sound, 48000)
        subprocess.run([canvas, "-t", "-i", root / "in.wav", "-o", root / "out.png"], check=True)
        out_image = np.asarray(PIL.Image.open(root / "out.png"))

        assert np.any(out_image[:, :, :3] != 0)
        np.testing.assert_allclose(out_image[:, :, 1], 0)
        np.testing.assert_allclose(out_image[:, :, 0], out_image[:, :, 2])

def test_stereo_sound_to_image(canvas, stereo_sound):
    """Converting a stereo sound to image produces a non-blank image with the following
    properties:
    - blue channel does not equal red channel
    - green channel is blank
    """
    with tempfile.TemporaryDirectory() as directory:
        root = pathlib.Path(directory)
        soundfile.write(root / "in.wav", stereo_sound, 48000)
        subprocess.run([canvas, "-t", "-i", root / "in.wav", "-o", root / "out.png"], check=True)
        out_image = np.asarray(PIL.Image.open(root / "out.png"))

        assert np.any(out_image[:, :, :3] != 0)
        np.testing.assert_allclose(out_image[:, :, 1], 0)
        assert np.any(out_image[:, :, 0] != out_image[:, :, 2])

def test_mono_sound_to_sound(canvas, mono_sound):
    """Converting sound to sound produces non-silent, stereo audio with both channels equal."""
    with tempfile.TemporaryDirectory() as directory:
        root = pathlib.Path(directory)
        soundfile.write(root / "in.wav", mono_sound, 48000)
        subprocess.run([canvas, "-t", "-i", root / "in.wav", "-o", root / "out.wav"], check=True)
        out_sound, rate = soundfile.read(root / "out.wav")
        assert out_sound.shape[1] == 2
        assert np.any(out_sound != 0)
        np.testing.assert_allclose(out_sound[:, 0], out_sound[:, 1])

def test_stereo_sound_to_sound(canvas, stereo_sound):
    """Converting sound to sound produces non-silent, stereo audio with channels different."""
    with tempfile.TemporaryDirectory() as directory:
        root = pathlib.Path(directory)
        soundfile.write(root / "in.wav", stereo_sound, 48000)
        subprocess.run([canvas, "-t", "-i", root / "in.wav", "-o", root / "out.wav"], check=True)
        out_sound, rate = soundfile.read(root / "out.wav")
        assert out_sound.shape[1] == 2
        assert np.any(out_sound != 0)
        assert np.any(out_sound[:, 0] != out_sound[:, 1])
