(ns redistore.core
  (:gen-class)
  (:require [taoensso.carmine :as car :refer (wcar)]))

(defmacro wcar* [server-conn & body] `(car/wcar ~server-conn ~@body))

(defn -main
  "I don't do a whole lot ... yet."
  [& args]
  (let [rlist (nth args 0 "redistore")
        host (nth args 1 "127.0.0.1")
        port (nth args 2 6379)
        server-conn {:pool {} :sepc {:host host :port port}}]
    (loop [line (read-line)]
      (wcar* server-conn (car/lpush rlist line))
      (recur (read-line)))))
