# Image search with vector representations (K23γ, Assignment 1)

Bag-of-visual-words image retrieval on HPatches with MIRFlickr-25K distractors, comparing
Exact Search, LSH, Hypercube, IVFFlat and IVFPQ.

## Requirements

- Linux, g++ with C++17, make, pkg-config
- OpenCV 4.4+ (SIFT is in `features2d`)

On Ubuntu:

```
sudo apt install build-essential pkg-config libopencv-dev unzip
```

## Data

```
mkdir -p ~/datasets && cd ~/datasets
wget https://huggingface.co/datasets/vbalnt/hpatches/resolve/main/hpatches-sequences-release.zip
wget https://press.liacs.nl/mirflickr/mirflickr25k.v3b/mirflickr25k.zip
unzip hpatches-sequences-release.zip && unzip mirflickr25k.zip
```

`-hp` takes the directory containing the sequence folders (`i_ajuntament/1.ppm`, ...) and
`-mir` the directory containing `im1.jpg` ... `im25000.jpg`.

## Build

```
make          # produces ./search
make clean
```

## Run

```
./search -hp ~/datasets/hpatches-sequences-release -mir ~/datasets/mirflickr \
  -split split.txt -vocab 256 -S 500 -D 10000 \
  -set validation -o output.txt \
  -ivfflat -kclusters 100 -nprobe 10 -seed 1
```

### Common parameters

| Option | Meaning | Default |
|---|---|---|
| `-hp <path>` | HPatches directory | required |
| `-mir <path>` | MIRFlickr-25K directory | required |
| `-split <file>` | train/validation/test split file | required |
| `-o <file>` | output file | required |
| `-vocab <int>` | number K of SIFT k-means centers | 256 |
| `-S <int>` | max SIFT descriptors per training image for k-means | 500 |
| `-D <int>` | number of MIRFlickr distractor images | 1000 |
| `-set <validation\|test>` | query set | validation |
| `-seed <int>` | random seed | 1 |
| `-cache <dir>` | directory for the cached vocabulary and image vectors (extra option) | `cache` |

### Cache

SIFT extraction is the slowest step, so the vocabulary and the image vectors are saved in
`-cache` and reused by later runs. File names contain K, S, the seed and a hash of the split
file, so changing any of them builds a new cache. The MIRFlickr cache grows as needed: a run
with a larger `-D` only processes the extra images. Delete the directory to rebuild everything.

### Methods

Exactly one of the following is required, followed by its optional parameters.

| Method | Options (default) |
|---|---|
| `-exact` | none |
| `-lsh` | `-k` (4), `-L` (5), `-w` (0.05) |
| `-hypercube` | `-kproj` (10), `-w` (0.05), `-M` (500), `-probes` (10) |
| `-ivfflat` | `-kclusters` (⌈√n⌉), `-nprobe` (5) |
| `-ivfpq` | `-kclusters` (⌈√n⌉), `-nprobe` (5), `-M` (16, must divide `-vocab`), `-nbits` (8) |

## Files

| File | Contents |
|---|---|
| `Makefile` | build rules |
| `split.txt` | HPatches train/validation/test split |
| `include/args.hpp`, `src/args.cpp` | command-line parsing and validation |
| `include/dataset.hpp`, `src/dataset.cpp` | split file reader, HPatches and MIRFlickr file listing |
| `include/sift_features.hpp`, `src/sift_features.cpp` | SIFT descriptor extraction with OpenCV |
| `include/kmeans.hpp`, `src/kmeans.cpp` | own k-means (k-means++ initialisation, Lloyd iterations) |
| `include/bovw.hpp`, `src/bovw.cpp` | vocabulary training (up to S descriptors per training image) and vectors x_I |
| `include/vector_set.hpp`, `src/vector_set.cpp` | storage of image vectors and their binary cache format |
| `include/features.hpp`, `src/features.cpp` | builds or loads the vocabulary and all image vectors, with caching |
| `include/distance.hpp` | squared Euclidean distance |
| `include/parallel.hpp` | parallel loop over all CPU cores with `std::thread` |
| `src/main.cpp` | entry point |
