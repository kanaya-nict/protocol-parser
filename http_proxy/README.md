# What?

This is a HTTP Proxy parser and redirector for SF-TAP Flow Abstracotr.
This program reads data from HTTP Proxy interface of Flow Abstractor and
redirects into loopback interface of Flow Abstractor.

## Installation

Dependencies
- [Leiningen](http://leiningen.org/)
- [Apache Maven](http://maven.apache.org/)
- [junixsocket](https://code.google.com/p/junixsocket/)

### Ubuntu 14.04

    $ sudo apt-get install leiningen maven
    $ git clone https://github.com/stap-project/protocol-parser.git
    $ cd protocol-parser/javaclass
    $ ./install.sh
    $ sudo mkdir -p /opt/newsclub/lib-native
    $ sudo cp linux/libjunixsocket-linux-1.5-amd64.so /opt/newsclub/lib-native

### Build junixsocket from source and install

    $ cd protocol-parser/javaclass
    $ tar cjfv junixsocket-1.3_stap.tar.bz2
    $ cd junixsocket-1.3_stap
    $ ant
    $ mvn install:install-file -Dfile=junixsocket-1.3.jar
    -DgroupId=local -DartifactId=junixsocket -Dversion=1.3
    -Dpackaging=jar -DgeneratePom=true
    $ sudo cp linux/libjunixsocket-linux-1.5-amd64.so /opt/newsclub/lib-native

## Usage

Run by using lein.

    $ cd protocol-parser/http
    $ lein deps
    $ lein run [path_to_http_if] [path_to_loopback_if]

Build jar and run by using java.

    $ lein compile
    $ lein uberjar
    $ java -jar http_proxy-0.1.0-sftap-standalone.jar [path_to_http_if] [path_to_loopback_if]

## Options

The first argument is a path to unix domain socket of HTTP Proxy interface.
If not specified, "/tmp/sf-tap/tcp/http_proxy" is used by default.

The second argument is a path to unix domain socket of loopback interface.
If not specified, "/tmp/sf-tap/tcp/loopback7" is used by default.

## License

Copyright © 2014 Yuuki Takano <ytakanoster@gmail.com>

Distributed under the 3-Clause BSD License
