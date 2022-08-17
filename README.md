# PaCHash-Experiments

This repository contains benchmarks for PaCHash.
The setup of the development machine and plot generation tools are rather inconventient, so we provide a docker file for easy reproducibility.
While the experiments in our paper are run without Docker, we recommend using the docker file for easier setup.
If you want to run the experiments outside a docker container, you can use the Dockerfile as a reference for setting up the tools.

To build the docker image with a pre-compiled PaCHash install and all benchmark utilities, run the following command.
Compiler warnings in competitors' code are expected and should not prevent building the docker image.
Building the container takes about 10 minutes.

```
docker build -t pachash .
```

The run scripts we provide are simplified versions of the plots from our paper.
They run fewer iterations and output fewer data points, but run faster.
You can modify the scripts in `scripts/dockerVolume` without re-building the container, but if you want to modify the c++ code, you need to re-build it.
Different experiments can be started by changing the launch command of the docker image.
The path `/path/to/ssd` should should be changed to a folder on the host system that is stored on a fast SSD.
If you do not mount the volume, docker's (comparably slow) layer filesystem is used.


| Figure in paper | Launch command                | Estimated runtime  |
| :-------------- | :---------------------------- | :----------------- |
| 2               | /opt/dockerVolume/figure-2.sh | 10 minutes         |
| 4               | /opt/dockerVolume/figure-4.sh | 20 minutes         |

```
docker run --interactive --tty -v "$(pwd)/scripts/dockerVolume:/opt/dockerVolume" -v "/path/to/ssd:/opt/testDirectory" pachash /opt/dockerVolume/figure-4.sh
```

### License
The competitors in the `external` folder are licensed under their respective license.
The benchmark code is licensed under the [GPLv3](/LICENSE).
If you use the project in an academic context or publication, please cite our paper.
