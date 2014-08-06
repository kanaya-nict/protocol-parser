#!/bin/sh

mvn install:install-file -Dfile=junixsocket-1.3.jar -DgroupId=local -DartifactId=junixsocket -Dversion=1.3 -Dpackaging=jar -DgeneratePom=true
