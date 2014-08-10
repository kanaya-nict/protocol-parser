(ns http.core
  (:gen-class)
  (:import [org.newsclub.net.unix
            AFUNIXSocket AFUNIXSocketAddress AFUNIXSocketException]))

(use '[clojure.string :only (split)])

(def bufsize 65536)
(def numpool (* 2 (.. Runtime getRuntime availableProcessors)))

(defn read_header [rdr]
  (let [line (.readLine rdr)]
    (if (empty? line)
      (do (println "remote socket was closed")
          (System/exit 0))
      (apply hash-map (flatten (map #(split % #"=") (split line #",")))))))

(defn read_data [rdr header parser session idx]
  (let [len (read-string (header "len"))
        from (read-string (header "from"))
        match (header "match")
        id (dissoc header "event" "from" "match" "len")
        pid (session id)
        buf (make-array Byte/TYPE len)
        rlen (.read rdr buf 0 len)]
    (if (= rlen -1)
      (do (println "remote socket was closed")
          (System/exit 0))
      (if (not (nil? pid))
        (let [pagent (nth parser pid)]
          (send pagent (fn [curr]
                         (do (println (str "in DATA, len = " len))
                             (println (str header))
                             (println (apply str (map #(format "%02x" (bit-and 0xff (int %))) buf)))
                             curr))))))
      [session idx]))

(defn flow_created [rdr header parser session idx]
  (let [id (dissoc header "event")
        pagent (nth parser idx)]
    (send pagent (fn [curr]
                   (do (println "flow created")
                       (println (str id))
                       (assoc curr id {:id id
                                       :client {:data '()
                                                :state :method}
                                       :server {:data '()
                                                :state :code}}))))
    [(assoc session id idx) (mod (inc idx) numpool)]))

(defn flow_destroyed [rdr header parser session idx]
  (let [id (dissoc header "event")
        pid (session id)]
    (if (not (nil? pid))
      (let [pagent (nth parser pid)]
        (send pagent (fn [curr] (do (println "flow destroyed")
                                    (println pid)
                                    (println (str id))
                                    (println (dissoc curr id))
                                    (dissoc curr id))))))
    [(dissoc session id) idx]))

(defn uxreader [sock]
  (with-open [rdr (java.io.DataInputStream. (.getInputStream sock))]
    (try
      (loop [parser (take numpool (repeatedly #(agent {})))
             session {}
             idx 0]
        (let [header (read_header rdr)
              [s i] (condp = (header "event")
                      "CREATED" (flow_created rdr header parser session idx)
                      "DATA" (read_data rdr header parser session idx)
                      "DESTROYED" (flow_destroyed rdr header parser session idx))]
          (recur parser s i)))
      (catch java.io.IOException e
        (println "error: couldn't read from socket")
        (System/exit 1)))))

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
