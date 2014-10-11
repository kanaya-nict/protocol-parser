(defproject http "0.1.0-sftap"
  :description "HTTP Parser for SF-TAP"
  :url "https://github.com/SF-TAP/protocol-parser"
  :license {:name "3-Clause BSD License"
            :url "http://opensource.org/licenses/BSD-3-Clause"}
  :dependencies [[org.clojure/clojure "1.6.0"]
                 [org.clojure/data.json "0.2.5"]
                 [net.n01se/clojure-jna "1.0.0"]]
  :main ^:skip-aot http.core
  :target-path "target/%s"
  :profiles {:uberjar {:aot :all}})
