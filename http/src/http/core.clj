(ns http.core
  (:gen-class)
  (:import [org.newsclub.net.unix AFUNIXServerSocket]))

(defn -main
  "I don't do a whole lot ... yet."
  [& args]
  (let [uxpath (nth args 0 "/tmp/stap/tcp/http")]
    (println uxpath)))
