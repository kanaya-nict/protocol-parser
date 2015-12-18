# BitTorrent DHT

This is a parser for BitTorrent DHT protocol.
See [http://www.bittorrent.org/beps/bep_0005.html](http://www.bittorrent.org/beps/bep_0005.html "DHT Protocol").

## install dependencies

    $ cabal install attobencode json base64-bytestring network socket

## compile and run

    $ ghc sftap_torrent_dht.hs
    $ ./sftap_torrent_dht

## output

The values of the keys of "ip", "y", "q",and "nodes" are strings.
Other string values are encoded with BASE64.

    {
      "src": {"ip": string, "port": integer},
      "dst": {"ip": string, "port": integer},
      "time": UNIX epoch time,
      "bencode":
      {
        "ip": string,
        "y": string,
        "q": string,
        "r": {
          "id": BASE64 string,
          "nodes": [[string, integer], [string, integer], ...]
        },
        "t": BASE64 string,
        "v": BASE64 string,
        ...
      }
    }
