(ns redistore.core
  (:gen-class)
  (:require [taoensso.carmine :as car :refer (wcar)]))

(def server1-conn {:pool {} :sepc {:host "127.0.0.1" :port 6379}})
(defmacro wcar* [& body] `(car/wcar server1-conn ~@body))

(defn -main
  "I don't do a whole lot ... yet."
  [& args]
  (loop [rlist (nth args 0 "redistore")
         line (read-line)]
    (wcar* (car/lpush rlist line))
    (recur rlist (read-line))))
