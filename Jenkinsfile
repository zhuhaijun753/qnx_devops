pipeline {

    agent {
        docker {
            image 'repo-manager.cloud.zf-world.com/proai_cv-docker-dev-local-frd/zf/proai-cv-nxp-qnx:1.4-alpha'
        }
    }

    stages {
        stage ('Test Docker Image') {
            steps{
                sh 'uname -a'
            }
        }
    }
          
}
