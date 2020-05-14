# OpenShift

## Configuration of Secrets

The following secrets shall be configured to trigger the build of a Docker image:

Name                       | Type                             | Description
---                        | ---                              | ---
`proai-cv.docker.registry` | `kubernetes.io/dockerconfigjson` | Access to the Docker registry (required to pull and push Docker images)
`proai-cv.git`             | `kubernetes.io/basic-auth`       | Read access to the Git repositories of the project **ProAI for Commercial Vehicles**

## Input files

The following files are required to build a Docker image for the project:

Name                                                             | Origin
---                                                              | ---
`cuda-repo-cross-qnx-10-2-local-10.2.58_1.0-1_all.deb`           | Downloaded from [NVOnline](https://partners.nvidia.com/)
`cuda-repo-ubuntu1804-10-2-local-10.2.58-430.40_1.0-1_amd64.deb` | Downloaded from [NVOnline](https://partners.nvidia.com/)
`drive-t186ref-foundation-5.1.9.0-16666715-release-pdk.run`      | Downloaded from [NVOnline](https://partners.nvidia.com/)
`drive-t186ref-foundation-5.1.9.0-16666715-release-sdk.run`      | Downloaded from [NVOnline](https://partners.nvidia.com/)
`drive-t186ref-qnx-5.1.9.0-16744669-cuda-qnx64-native-libs.run`  | Downloaded from [NVOnline](https://partners.nvidia.com/)
`drive-t186ref-qnx-5.1.9.0-16744669-cudnn-qnx.run`               | Downloaded from [NVOnline](https://partners.nvidia.com/)
`drive-t186ref-qnx-5.1.9.0-16744669-driver-pdk.run`              | Downloaded from [NVOnline](https://partners.nvidia.com/)
`drive-t186ref-qnx-5.1.9.0-16744669-driver-sdk.run`              | Downloaded from [NVOnline](https://partners.nvidia.com/)
`drive-t186ref-qnx-5.1.9.0-16744669-qnx-early-access.run`        | Downloaded from [NVOnline](https://partners.nvidia.com/)
`drive-t186ref-qnx-5.1.9.0-16744669-tensorrt-qnx.run`            | Downloaded from [NVOnline](https://partners.nvidia.com/)
`qnx-sdp-700-drive-os-5.1.9.P201908021237.tar.xz`                | Manually created (see [Create QNX SDP Offline Package](#create-qnx-sdp-offline-package) for details)

All input files are uploaded to the [binary repository](https://repo-manager.cloud.zf-world.com/artifactory/webapp/#/artifacts/browse/tree/General/proai_cv-generic-dev-local-frd).

### Create QNX SDP Offline Package<a name="create-qnx-sdp-offline-package"></a>

The file `qnx-sdp-700-drive-os-5.1.9.P201908021237.tar.xz` is created by following the instructions that are described in the [SDK Manager Quickstart Guide](https://docs.nvidia.com/drive/drive_os_5.1.9.0L/drive-qsg/qnx-setup/index.html).

#### Preconditions

The following preconditions shall be fulfilled:

- Linux OS installed (verified with Ubuntu 18.04 LTS)
- QNX Software Center installed (verified with QNX Software Center 1.6.0)
- [myQNX](https://www.qnx.com/account/) user account available
- Activated build server license for the user account
- Patchset file `drive-t186ref-qnx-5.1.9.0-sdp-patchset.qpkg` downloaded from [NVOnline](https://partners.nvidia.com/)

#### Import of Offline Package

Proceed the following instruction to create the file `qnx-sdp-700-drive-os-5.1.9.P201908021237.tar.xz`:

- Remove the content of folder `${HOME}/.qnx/swupdate/dropins/`
- Launch the QNX Software Center application
- On the welcome screen select `Import Offline Package`
- Select the package file `drive-t186ref-qnx-5.1.9.0-sdp-patchset.qpkg` and use `Create new installation`
- Choose an installation folder, e.g. `$HOME/qnx700`
- Set `Update Policy` to `Conservative`
- Check `Install experimental packages`
- Uncheck `Install debug symbols` if activated (optional, but recommended to shrink the size of the archive)
- Choose `aarch64le` as target architecture
- Ensure that the correct items are selected (for details check the [SDK Manager Quickstart Guide](https://docs.nvidia.com/drive/drive_os_5.1.9.0L/drive-qsg/qnx-setup/index.html)) and proceed the installation
- Once the installation is finished, archive the files, e.g. use `cd $HOME && tar -I "xz -9" -cvf qnx-sdp-700-drive-os-5.1.9.P201908021237.tar.xz qnx700`

## Create a Docker image

At present Docker images need to be created manually.

The following steps are required to trigger the build of a new Docker image:

- Ensure that the latest version of the `Dockerfile` is pushed to the remote repository
- Log in to the [OpenShift Container Platform](https://mgmt.krl-ocpd.emea.zf-world.com/)
- Select the project, e.g. `proai_cv`
- On the overview page select `Import YAML / JSON`
- Choose the file `buildconfig.yaml` from a local clone of the repository (the file is located under the folder `/build/openshift`)
- Process the template and select a proper `Image Tag`
- Once the build config has been created, navigate to the build `proai-cv-nvidia-qnx-5.1.9` and click `Start Build`
