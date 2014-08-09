(ns http.core
  (:gen-class)
  (:import [org.newsclub.net.unix
            AFUNIXSocket AFUNIXSocketAddress AFUNIXSocketException]))

(use '[clojure.string :only (split)])

(def bufsize 65536)
(def session (ref {}))

(defn read_header [rdr]
  (let [line (.readLine rdr)]
    (if (empty? line)
      (do (println "remote socket was closed")
          (System/exit 0))
      (apply hash-map (flatten (map #(split % #"=") (split line #",")))))))

(defn read_data [rdr header]
  (let [len (read-string (header "len"))
        buf (make-array Byte/TYPE len)
        ret (.read rdr buf 0 len)]
    (if (= ret -1)
      (do (println "remote socket was closed")
          (System/exit 0))
      (do (println (str "in DATA, len = " len))
          (println (apply str (map #(format "%02x" (bit-and 0xff (int %))) buf)))))))

(defn flow_created [rdr header]
  (let [h (dissoc header "event" "len")]
    (dosync (alter session assoc h {:state :method :data '()}))
    (println (str @session))
    (println "flow created")))

(defn flow_destroyed [rdr header]
  (let [h (dissoc header "event" "len")]
    (dosync (alter session dissoc h))
    (println (str @session))
    (println "flow destroyed")))

(defn uxreader [sock]
  (with-open [rdr (java.io.DataInputStream. (.getInputStream sock))]
    (let [buf (byte-array bufsize)]
      (loop []
        (let [header (read_header rdr)]
          (println (str header))
          (try
            (condp = (header "event")
              "DATA" (read_data rdr header)
              "CREATED" (flow_created rdr header)
              "DESTROYED" (flow_destroyed rdr header))
            (catch java.io.IOException e
              (println "error: couldn't read from socket")
              (System/exit 1)))
        (recur))))))

(defn uxconnect [uxpath]
  (with-open [sock (AFUNIXSocket/newInstance)]
    (try
      (.connect sock (AFUNIXSocketAddress. (clojure.java.io/file uxpath)))
      (catch AFUNIXSocketException e
        (println (str "error: couldn't open \"" uxpath "\""))
        (System/exit 1)))
    (println (str "connected to " uxpath))
    (uxreader sock)))

(defn -main
  "I don't do a whole lot ... yet."
  [& args]
  (let [uxpath (nth args 0 "/tmp/stap/tcp/http")]
    (uxconnect uxpath)))
