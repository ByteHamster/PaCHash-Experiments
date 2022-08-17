# PaCHash Experiments
This repository contains our reproducibility artifacts for PaCHash.
Due to the plethora of dependencies required by our competitors, we provide an easy to use Docker image to quickly reproduce our results.
Alternatively, you can look at the `Dockerfile` to see all libraries, tools, and commands necessary to compile PaCHash and its competitors.
The source code used in the experimental evaluation can be found in `external` (patches that allow us to measure the running time and memory consumption of our competitors are stored in `patches`).

## Building the Docker Image

Run the following command to build the Docker image.
Building the image takes about 10 minutes, as some packages (including LaTeX for the plots) have to be installed.

```bash
docker build -t pachash .
```

Some compiler warnings (red) are expected when building competitors and will not prevent building the image or running the experiments.
Please ignore them!

## Running the Experiments
Due to the long total running time of all experiments in our paper, we provide run scripts for a slightly simplified version of the experiments.
They run fewer iterations and output fewer data points.
Also, we omit the experiments requiring real world data due to the size of the data (about 50GiB) and the experiments for variable size objects as the results are the same.
If you are interested in these experiments, please contact the authors.

You can modify the benchmarks scripts in `scripts/dockerVolume` if you want to change the number of runs or data points.
This does not require the Docker image to recompile.
Different experiments can be started by using the following command:

```bash
docker run --interactive --tty -v "$(pwd)/scripts/dockerVolume:/opt/dockerVolume" -v "/<path_to_ssd>:/opt/testDirectory" pachash /opt/dockerVolume/figure-<number>.sh
```

Here, `<path_to_sdd>` has to be changed to a folder on the host system on the disk you want to run the experiments on.
`<number>` should be either `2` or `4`, depending on the experiment you want to run.
The number also refers to the figure in the paper.

| Figure in paper | Launch command                | Estimated runtime  |
| :-------------- | :---------------------------- | :----------------- |
| 2               | /opt/dockerVolume/figure-2.sh | 10 minutes         |
| 4               | /opt/dockerVolume/figure-4.sh | 20 minutes         |

The resulting plots can be found in `scripts/dockerVolume` and are called `figure-<number>.pdf`.
