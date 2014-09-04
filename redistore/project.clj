(defproject redistore "0.1.0-sftap"
  :description "Store Standard Input to Redis"
  :url "http://example.com/FIXME"
  :license {:name "3-Clause BSD License"
            :url "http://www.eclipse.org/legal/epl-v10.html"}
  :dependencies [[org.clojure/clojure "1.6.0"]
                 [com.taoensso/carmine "2.7.0"
                  :exclusions [org.clojure/clojure]]]
  :main ^:skip-aot redistore.core
  :target-path "target/%s"
  :profiles {:uberjar {:aot :all}})
