import numpy as np

if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument("size", type=int)
    args = parser.parse_args()

    t = np.linspace(0, 1, args.size, endpoint=False)
    sine = np.sin(2 * np.pi * t)

    print(", ".join([str(x) for x in sine]))
