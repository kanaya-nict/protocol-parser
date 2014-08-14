(ns http.core
  (:gen-class)
  (:import [org.newsclub.net.unix
            AFUNIXSocket AFUNIXSocketAddress AFUNIXSocketException]))

(use '[clojure.string :only (split)])
(use 'clojure.pprint)

;(def numpool (+ 2 (.. Runtime getRuntime availableProcessors)))
(def numpool 1)
(def top {:client :method :server :responce})

(defn bytes2str [bytes]
  (apply str (map char bytes)))

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
        ;;(println "line: " (apply str (map char line)))
        [line (drop (inc idx) data)]))))

(defn parse_method [line]
  (try
    (let [method (split (bytes2str line) #" ")]
      (if (= 3 (count method))
        {:method (nth method 0)
         :uri (nth method 1)
         :ver (nth method 2)}
        nil))
    (catch IllegalArgumentException e nil)))

(defn parse_responce [line]
  (try
    (let [idx1 (.indexOf line (byte \space))
          ver (bytes2str (take idx1 line))
          line2 (drop (inc idx1) line)
          idx2 (.indexOf line2 (byte \space))
          code (bytes2str (take idx2 line2))
          msg (bytes2str (drop (inc idx2) line2))]
      {:ver ver :code code :msg msg})
    (catch IllegalArgumentException e nil)))

(defn parse_header [line]
  (if (= line '())
    :end
    (try
      (let [idx (.indexOf line (byte \:))]
        (if (= idx -1)
          nil
          [(clojure.string/lower-case (bytes2str (take idx line)))
           (bytes2str (drop (+ idx 2) line))]))
      (catch IllegalArgumentException e nil))))

(defn get_content_len [flow id peer]
  (Integer. ((get-in flow [id peer :header]) "content-length")))

(defn has_body [flow id peer]
  (contains? (get-in flow [id peer :header]) "content-length"))

(defn is_chunked [flow id peer]
  (= (get-in flow [id peer :header "transfer-encoding"]) "chunked"))

(defn parse_headers [flow id peer]
  (let [[line new_data] (take_line (get-in flow [id peer :data]))]
    (if (nil? line)
      [false flow] ;; no lines
      (let [header (parse_header line)]
        (println header)
        (condp = header
          ;; failed to parse a header
          nil [false (assoc-in flow [id :clinet :state] :error)]

          ;; no more headers
          :end (cond
                ;; have body
                (has_body flow id peer)
                (try
                  [true
                   (-> flow
                       (assoc-in [id peer :data] new_data)
                       (assoc-in [id peer :state] :body)
                       (assoc-in [id peer :remain] (get_content_len flow id peer)))]
                  (catch NumberFormatException e
                    [false (assoc-in flow [id peer :state] :error)]))

                ;; have chunked body
                (is_chunked flow id peer)
                [true (-> flow
                          (assoc-in [id peer :state] :chunk-len)
                          (assoc-in [id peer :data] new_data))]

                ;; no body and chunk
                :else [true
                       (-> flow
                           (assoc-in [id peer :data] new_data)
                           (assoc-in [id peer :state] (top peer)))])

          ;; have more headers
          [true
           (-> flow
               (assoc-in [id peer :data] new_data)
               (assoc-in [id peer :header (nth header 0)] (nth header 1)))])))))

(defn skip_body [flow id peer]
  (let [remain (get-in flow [id peer :remain])
        data (get-in flow [id peer :data])
        len (count data)]
    (println "remain = " remain)
    (if (>= len remain)
      [true (-> flow
                (assoc-in [id peer :data] (drop remain data))
                (assoc-in [id peer :state] (top peer)))]
      [false (-> flow
                 (assoc-in [id peer :remain] (- remain len))
                 (assoc-in [id peer :data] '()))])))

(defn parse [flow0 id buf peer]
  (loop [flow (assoc-in flow0 [id peer :data]
                        (concat (get-in flow0 [id peer :data]) buf))]
    (condp = (get-in flow [id peer :state])
      :method (let [[line new_data] (take_line
                                     (get-in flow [id peer :data]))]
                (if (nil? line)
                  flow
                  (let [method (parse_method line)]
                    (println "method = " method)
                    (if (nil? method)
                      (assoc-in flow [id peer :state] :error)
                      (recur
                       (-> flow
                           (assoc-in [id peer :method] method)
                           (assoc-in [id peer :data] new_data)
                           (assoc-in [id peer :state] :header)))))))
      :responce (let [[line new_data] (take_line
                                       (get-in flow [id peer :data]))]
                  (if (nil? line)
                    flow
                    (let [responce (parse_responce line)]
                      (println "responce: " responce)
                      (if (nil? responce)
                        (assoc-in flow [id peer :state] :error)
                        (recur
                         (-> flow
                             (assoc-in [id peer :responce] responce)
                             (assoc-in [id peer :data] new_data)
                             (assoc-in [id peer :state] :header)))))))
      :header (let [[is_recur new_flow] (parse_headers flow id peer)]
                (condp = is_recur
                  false new_flow
                  true (recur new_flow)))
      :body (let [[is_recur new_flow] (skip_body flow id peer)]
              (condp = is_recur
                false new_flow
                true (recur new_flow)))
      :chunk-len flow
      :chunk-body flow
      :error flow)))

(defn parse_http [flow header id buf]
  (do
;    (println (str "in DATA, len = " (alength buf)))
;    (println (str header))
;    (println (apply str (map #(format "%02x" (bit-and 0xff (int %))) buf)))
;    (pprint buf)
    (condp = (header "match")
      "up" (parse flow id buf :client)
      "down" (parse flow id buf :server))))

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
                                                :state :responce
                                                :responce nil
                                                :header {}}}))))
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
