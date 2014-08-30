(defproject http_proxy "0.1.0-sf-tap-http-proxy"
  :description "Redirector of HTTP Proxy for SF-TAP"
  :url "https://github.com/SF-TAP/protocol-parser"
  :license {:name "3-Clause BSD License"
            :url "http://opensource.org/licenses/BSD-3-Clause"}
  :dependencies [[org.clojure/clojure "1.6.0"]
                 [org.clojure/data.json "0.2.5"]
                 [local/junixsocket "1.3"]]
  :main ^:skip-aot http-proxy.core
  :target-path "target/%s"
  :profiles {:uberjar {:aot :all}})
