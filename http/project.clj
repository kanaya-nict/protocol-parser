(defproject http "0.1.0-stap-http"
  :description "HTTP Parser for STAP"
  :url "http://example.com/FIXME"
  :license {:name "3-Clause BSD License"
            :url "http://opensource.org/licenses/BSD-3-Clause"}
  :dependencies [[org.clojure/clojure "1.6.0"]
                 [local/junixsocket "1.3"]]
  :main ^:skip-aot http.core
  :target-path "target/%s"
  :profiles {:uberjar {:aot :all}})
