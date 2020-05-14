################################################
# First build stage including user credentials #
################################################

# As long as OpenShift does not support BuildKit, we need to copy the secrets
# into the image. Multi-stage builds are used in order to hide the secrets in
# the final image.

# --- Base image

FROM repo-manager.cloud.zf-world.com/proai_cv-docker-dev-local-frd/zf/zf-ubuntu:bionic-20200219 AS provider

# --- Injection of secrets (usernames and access tokens)

#COPY ./secrets /secrets


# --- Installation of additional packages

# curl (7.58.0-2ubuntu3.8) is already installed and required to fetch files
# from the binary repository

# script with sudo commands

# tar (1.29b-2ubuntu0.1) is already installed and required to extract files

# xz-utils required to extract files from xz archives

# Use version pinning for reproducibility

RUN DEBIAN_FRONTEND=noninteractive \
  apt-get install --assume-yes --no-install-recommends \
  xz-utils=5.2.2-1.3 \
  sudo=1.8.21p2-3ubuntu1.2

# --- Configuration of curl

# Using --user option may show the password on error messages. This is not
# desired (e.g. think about a log that is stored on Jenkins). Therefore we use
# the --netrc option and store the credentials in the file .netrc.

RUN echo "\
  machine repo-manager.cloud.zf-world.com\n\
    login z0010648\n\
    password 7493251Ckh"\
  > ~/.netrc
RUN chmod u=rw,go= ~/.netrc

# --- Download of QNX Software Development Platform (SDP)

RUN curl --fail --netrc \
  https://repo-manager.cloud.zf-world.com/artifactory/proai_cv-generic-dev-local-frd/qnx/qnx-sdp/qnx-sdp-700-safetympu/1.0.0.20200505/qnx-sdp-700-safetympu-1.0.0.20200505.tar.xz \
  --output /tmp/qnx700.tar.xz
RUN tar -xf /tmp/qnx700.tar.xz -C /opt/


# #############################################
# # Final build stage without any credentials #
# #############################################

# --- Base image

FROM repo-manager.cloud.zf-world.com/proai_cv-docker-dev-local-frd/zf/zf-ubuntu:bionic-20200219

# --- Metadata

LABEL \
  maintainer="ZF Friedrichshafen AG" \
  io.openshift.build.commit.author="ZF Friedrichshafen AG" \
  description="Docker image for NXP S32G274A QNX BSP "

# --- Set environment variables

ENV HOME=/home/jenkins

# --- Installation of additional packages

RUN apt-get update --fix-missing

# Use version pinning for reproducibility
RUN DEBIAN_FRONTEND=noninteractive \
  apt-get install --assume-yes --no-install-recommends \
  apt-utils=1.6.12 \
  sudo=1.8.21p2-3ubuntu1.2 \
  xdg-utils=1.1.2-1ubuntu2.3 \
  software-properties-common=0.96.24.32.12 \
  openjdk-8-jdk=8u252-b09-1~18.04 \
  openssh-client=1:7.6p1-4ubuntu0.3 \
  git=1:2.17.1-1ubuntu0.7 \
  vim-common=2:8.0.1453-1ubuntu1.3 \
  cpio=2.12+dfsg-6ubuntu0.18.04.1 \
  expect=5.45.4-1 \
  locales=2.27-3ubuntu1 \
  liblz4-tool=0.0~r131-2ubuntu3 \
  libgtk-3-0=3.22.30-1ubuntu4 \
  libgtk-3-dev=3.22.30-1ubuntu4 \
  curl=7.58.0-2ubuntu3.8 \
  gcc=4:7.4.0-1ubuntu2.3 \
  g++=4:7.4.0-1ubuntu2.3 \
  cmake=3.10.2-1ubuntu2.18.04.1 \
  python=2.7.15~rc1-1 \
  python3-setuptools=39.0.1-2 \
  pycodestyle=2.3.1-2 \
  pyflakes=1.6.0-1 \
  python3-pip=9.0.1-2.3~ubuntu1.18.04.1 \
  gpg-agent=2.2.4-1ubuntu1.2 \
  shellcheck=0.4.6-1 \
  zip=3.0-11build1 \
  ninja-build=1.8.2-1


# --- Configuration of locale

# Install locale for en_US
RUN locale-gen en_US.UTF-8

# Update the locales to en_US.UTF-8. Unfortunately update-locale doesn't work
# here as it requires a reboot, so just set the environment variables manually.

# LANGUAGE = Set language to use en_US (fallbacks may be defined with ':'
# syntax, e.g. en_US:de_DE)
ENV LANGUAGE en_US

# LC_ALL = Set language to be used if LANGUAGE does not apply
ENV LC_ALL en_US.UTF-8

# LANG = Set language to be used if neither LANGUAGE nor LC_ALL apply
ENV LANG en_US.UTF-8

# --- Configuration and update of PyPI

# Configure PyPI to use the internal ZF server as index
ADD contrib/pip/pip.conf $HOME/.pip/
RUN pip3 install pip==20.1


# --- Installation of Conan

RUN pip3 install conan==1.24.0

# Add already installed ZF certificates to conan
RUN conan remote clean && \
  cat /usr/local/share/ca-certificates/* >> $HOME/.conan/cacert.pem

# --- Installation of QNX Software Development Platform (SDP)
# copy QNX Software Development Platform from provider image to final image

COPY --from=provider /opt/qnx700 /opt/qnx700


# Ensure QNX environment is set up properly for non-interactive shells. This
# file is also called indirectly for interactive shells, so there is no need to
# duplicate it in /etc/profile.

RUN echo "source /opt/qnx700/qnxsdp-env.sh" >> /etc/bash.bashrc

# --- Jenkins JNLP client files

ADD contrib/bin/* /usr/local/bin/

# --- Access rights

# Change access rights for files and folders, e.g. to allow writing for all
# user
RUN chmod ug=rw,o=r /etc/passwd
RUN chmod -R u=rwx,go=rx /usr/local/bin
RUN chmod -R go+w $HOME

# --- Configuration of entrypoint

# Run the Jenkins JNLP client
ENTRYPOINT ["/usr/local/bin/run-jnlp-client"]
