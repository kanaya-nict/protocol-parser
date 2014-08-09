(ns http.core
  (:gen-class)
  (:import [org.newsclub.net.unix
            AFUNIXSocket AFUNIXSocketAddress AFUNIXSocketException]))

(use '[clojure.string :only (split)])

(def bufsize 65536)

(defn read_header [rdr]
  (let [line (.readLine rdr)
        header (apply hash-map
                      (flatten (map #(split % #"=") (split line #","))))]
    header))

(defn read_data [rdr header]
  (let [len (read-string (header "len"))
        buf (make-array Byte/TYPE len)]
    (.read rdr buf 0 len)
    (println (str "in DATA, len = " len))
    (println (apply str (map #(format "%02x" (bit-and 0xff (int %))) buf)))))

(defn flow_created [rdr header]
  (println "flow created"))

(defn flow_destroyed [rdr header]
  (println "flow destroyed"))

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
