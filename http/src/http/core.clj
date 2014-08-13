(ns http.core
  (:gen-class)
  (:import [org.newsclub.net.unix
            AFUNIXSocket AFUNIXSocketAddress AFUNIXSocketException]))

(use '[clojure.string :only (split)])
(use 'clojure.pprint)

;(def numpool (+ 2 (.. Runtime getRuntime availableProcessors)))
(def numpool 1)

(defn read_header [rdr]
  (let [line (.readLine rdr)]
    (if (empty? line)
      (do (println "remote socket was closed")
          (System/exit 0))
      (apply hash-map (flatten (map #(split % #"=") (split line #",")))))))

(defn take_line [data]
  (let [idx (.indexOf data (byte \newline))]
    (if (= idx -1)
      [nil data]
      (let [line0 (take idx data)
            line (if (= (last line0) (byte \return))
                   (drop-last 1 line0)
                   line0)]
        ;(println "line: " (apply str (map char line)))
        [line (drop (inc idx) data)]))))

(defn parse_method [line]
  (try
    (let [method (split (apply str (map char line)) #" ")]
      (if (= 3 (count method))
        {:method (nth method 0)
         :uri (nth method 1)
         :ver (nth method 2)}
        nil))
    (catch IllegalArgumentException e nil)))

(defn parse_header [line]
  (if (= line '())
    :end
    (try
      (let [idx (.indexOf line (byte \:))]
        (if (= idx -1)
          nil
          [(clojure.string/lower-case (apply str (map char (take idx line))))
           (apply str (map char (drop (+ idx 2) line)))]))
      (catch IllegalArgumentException e nil))))

(defn has_body [flow id]
  (contains? (get-in flow [id :client :header]) "content-length"))

(defn is_chunked [flow id]
  (= (get-in flow [id :client :header "transfer-encoding"]) "chunked"))

(defn parse_client [flow0 id buf]
  (loop [flow (assoc-in flow0 [id :client :data]
                        (concat (get-in flow0 [id :client :data]) buf))]
    (condp = (get-in flow [id :client :state])
      :method (let [[line new_data] (take_line
                                     (get-in flow [id :client :data]))]
                (if (nil? line)
                  flow
                  (let [method (parse_method line)]
                    (if (nil? method)
                      (assoc-in flow [id :client :state] :error)
                      (do (println method)
                          (recur
                           (-> flow
                               (assoc-in [id :client :method] method)
                               (assoc-in [id :client :data] new_data)
                               (assoc-in [id :client :state] :header))))))))
      :header (let [[line new_data] (take_line
                                     (get-in flow [id :client :data]))]
                (if (nil? line)
                  flow
                  (let [header (parse_header line)]
                    (println header)
                    (condp = header
                      nil (assoc-in flow [id :clinet :state] :error)
                      :end (cond
                            (has_body flow id) (assoc-in flow [id :client :state] :body)
                            (is_chunked flow id) (assoc-in flow [id :client :state] :chunk-len)
                            :else (assoc-in flow [id :client :state] :method))
                      (recur (-> flow
                                 (assoc-in [id :client :data] new_data)
                                 (assoc-in [id :client :header (nth header 0)] (nth header 1))))))))
      :body flow
      :chunk-len flow
      :chunk-body flow
      :error flow)))

(defn parse_server [flow header id buf]
  flow)

(defn parse_http [flow header id buf]
  (do
;    (println (str "in DATA, len = " (alength buf)))
;    (println (str header))
;    (println (apply str (map #(format "%02x" (bit-and 0xff (int %))) buf)))
;    (pprint buf)
    (condp = (header "match")
      "up" (parse_client flow id buf)
      "down" (parse_server flow header id buf))))

(defn read_data [rdr header parser session idx]
  (let [len (read-string (header "len"))
        id (dissoc header "event" "from" "match" "len")
        pid (session id)
        buf (make-array Byte/TYPE len)
        rlen (.read rdr buf 0 len)]
    (if (= rlen -1)
      (do (println "remote socket was closed")
          (System/exit 0))
      (if (not (nil? pid))
        (let [pagent (nth parser pid)]
          (send pagent parse_http header id buf))))
    [session idx]))

(defn flow_created [rdr header parser session idx]
  (let [id (dissoc header "event")
        pagent (nth parser idx)]
    (send pagent (fn [flow]
                   (do (println "flow created")
                       (println (str id))
                       (assoc flow id {:client {:data '()
                                                :state :method
                                                :method nil
                                                :header {}}
                                       :server {:data '()
                                                :state :code}}))))
    [(assoc session id idx) (mod (inc idx) numpool)]))

(defn flow_destroyed [rdr header parser session idx]
  (let [id (dissoc header "event")
        pid (session id)]
    (if (not (nil? pid))
      (let [pagent (nth parser pid)]
        (send pagent (fn [flow] (do (println "flow destroyed")
                                    (println pid)
                                    (println (str id))
                                    (dissoc flow id))))))
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
