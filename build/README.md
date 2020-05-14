# Build the project

## bsp_working_dir

The bsp_working_dir work path of S32G274A EVB QNX BSP. 

bsp_working_dir/
├── images
├── install
├── Makefile
├── manifest
├── prebuilt
├── readme.txt
├── source.xml
└── src

## Manual builds

### Preconditions

The following preconditions shall be fulfilled in order to trigger a build:

- Linux OS installed (verified with Ubuntu 18.04 LTS)

- Docker installed (verified with Docker 19.03.8)

- Activated QNX license file `licenses` put into `$HOME/.qnx/license/`

- ZF certificates installed in order to access the infrastructure

### Docker image

Process the following steps to download the docker image that is required for the build:

- Log into Artifactory: https://repo-manager.cloud.zf-world.com/

- Create or fetch the [API token](https://repo-manager.cloud.zf-world.com/artifactory/webapp/#/profile)

- Log into the docker registry:

  ```
  docker login repo-manager.cloud.zf-world.com
  ```

- Provide your mail address as username and the API token as password

- Pull the docker image:

  ```
  docker pull repo-manager.cloud.zf-world.com/proai_cv-docker-dev-local-frd/zf/<proai-cv-nxp-qnx>:<tag>
  ```

  `tag` equals the tag of the Docker image that shall be pulled.

### Build instructions

Process the following steps in order to generate the build artifacts:

- Run docker with interactive shell:

  ```
  docker run \
    --interactive \
    --tty \
    --net=host \
    --entrypoint=/bin/bash \
    --volume $HOME/.qnx:/home/jenkins/.qnx \
    repo-manager.cloud.zf-world.com/proai_cv-docker-dev-local-frd/zf/<proai-cv-nxp-qnx>:<tag>
  ```

  `tag` equals the tag of the Docker image that has been pulled before.

