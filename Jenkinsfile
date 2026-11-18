// Same stages as GitHub Actions, for a Jenkins controller with Docker agents.
pipeline {
    agent {
        docker { image 'python:3.12-bookworm' }
    }
    options { timestamps() }
    stages {
        stage('Setup') {
            steps {
                sh 'apt-get update && apt-get install -y --no-install-recommends gcc make'
                sh 'pip install -r requirements-dev.txt'
            }
        }
        stage('Build') {
            steps { sh 'make' }
        }
        stage('Unit tests') {
            steps { sh 'make test' }
        }
        stage('Functional tests') {
            steps {
                sh 'MINI_UPF_BIN=$PWD/build/mini-upf python3 -m pytest -q tests/func --junitxml=build/func.xml'
            }
        }
        stage('Sanitizers') {
            steps { sh 'make clean && make SANITIZE=1 test functest' }
        }
    }
    post {
        always { junit allowEmptyResults: true, testResults: 'build/*.xml' }
    }
}
