# Setup Continuous Integration (CI)

## Jenkins

[Jenkins](https://jenkins.io/) is used to initiate a CI build for the Saftympu.

### Credentials

The `Jenkinsfile` requires the following credentials on the Jenkins server:

Name                     | Kind                   | Description
---                      | ---                    | ---
`proai-cv.artifatory`    | Username with password | Read-write access to the Artifactory server (required to search and publish artifacts)
`QNX_LICENSES_PROAI_CV`  | Secret file            | `licenses` file for the QNX SDP (requires a QNX SDP Enterprise Build Server Subscription Key). Can be copied from `.qnx/license/licenses` in the user's home directory once the license has been activated on a Windows or Linux PC

### Kubernetes

The Kubernetes plugin for Jenkins is required to run Docker images on a cloud server.

#### Plugin information

URL                                    | Version
---                                    | ---
https://plugins.jenkins.io/kubernetes/ | 1.24.1

#### Plugin configuration

The `oc` commands listed below requires a portable installation of the [client tools for OpenShift](https://github.com/openshift/origin/releases).

In Jenkins configure a Kubernetes cloud as follows:

Attribute                       | Value          | Description
---                             | ---            | ---
Name                            | `proai_cv`     | Fix identifier that is used in the Jenkinsfile to select the agent
Kubernetes URL                  | `<url>`        | URL to the OpenShift server, e.g. `https://mgmt.krl-ocpd.emea.zf-world.com`
Disable https certificate check | `true`         | _Optional:_ Only required if no server certificate is provided or needed
Kubernetes Namespace            | `<project>`    | The name of the OpenShift project, e.g. `ocp001077` (do not mix it up with the display Name). Use `oc get projects` to find out the name.
Credentials                     | `<credential>` | Jenkins credential for one of the service accounts. Use `oc get sa` to list all available accounts. Use `oc sa get-token <account>` to request the access token for a specific account.
Container Cleanup Timeout       | `45`           | Timeout in minutes to wait before a pod is cleaned up. Set it at least twice to the time that is needed for pulling a new docker image.

### Artifactory

The Artifactory plugin for Jenkins is required to push build artifacts to the binary repository.

#### Plugin information

URL                                     | Version
---                                     | ---
https://plugins.jenkins.io/artifactory/ | 3.6.1

#### Plugin configuration

In Jenkins configure an Artifactory server as follows:

Attribute                       | Value          | Description
---                             | ---            | ---
Server ID                       | `proai_cv`     | Fix identifier that is used in the Jenkinsfile to select the server
URL                             | `<url>`        | URL to the Artifactory server, e.g. `https://repo-manager.cloud.zf-world.com/`

## OpenShift

[OpenShift](https://www.openshift.com/) is used as Kubernetes platform for the Saftympu. A machine in OpenShift, a so called Pod, acts as agent for a single Jenkins build and is created on request, e.g. when a build requires a new agent.

### Configuration of Secrets

The following secrets shall be configured to run a CI build for the Saftympu:

Name                       | Type                             | Description
---                        | ---                              | ---
`proai-cv.docker.registry` | `kubernetes.io/dockerconfigjson` | Access to the Docker registry (required to pull and push Docker images)