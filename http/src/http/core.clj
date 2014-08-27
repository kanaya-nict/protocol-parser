(ns http.core
  (:gen-class)
  (:require [clojure.data.json :as json])
  (:import [org.newsclub.net.unix
            AFUNIXSocket AFUNIXSocketAddress AFUNIXSocketException]))

(use '[clojure.string :only (split)])
(use 'clojure.pprint)

(def numpool (+ 2 (.. Runtime getRuntime availableProcessors)))
(def output_agent (agent 0))
(def top {:client :method :server :responce})

(defn printerr [& args]
  (binding [*out* *err*]
    (apply println args)))

(defn push2result [flow id peer]
  (let [key (if (= peer :client)
            :method
            :responce)
        result (get-in flow [id peer :result])
        new_result (conj result
                         {:header (get-in flow [id peer :header])
                          key (get-in flow [id peer key])
                          :length (get-in flow [id peer :length])
                          :trailer (get-in flow [id peer :trailer])})]
    (-> flow
        (assoc-in [id peer :result] new_result)
        (assoc-in [id peer key] nil)
        (assoc-in [id peer :length] 0)
        (assoc-in [id peer :header] {})
        (assoc-in [id peer :trailer] {}))))

(defn get_addr [flow id peer]
  (if (= "1" (get-in flow [id peer :from]))
    [(id "ip1") (id "port1")]
    [(id "ip2") (id "port2")]))

(defn pop_peer_result [flow id peer]
  (let [result (get-in flow [id :peer :result])]
    (if (empty? result)
      (assoc-in flow [id peer :result] '())
      (let [[addr port] (get_addr flow id peer)]
        (map (fn [res]
               (let [new_res (-> res
                                 (assoc :ip addr)
                                 (assoc :port port))]
                 (send output_agent
                       (fn [flow]
                         (do
                           (println (json/write-str {peer new_res}))
                           0)))))
             result)
        (assoc-in flow [id peer :result] '())))))


(defn pop_result [flow id]
  (loop [client (get-in flow [id :client :result])
         server (get-in flow [id :server :result])]
    (if (or (empty? client) (empty? server))
      (-> flow
          (assoc-in [id :client :result] client)
          (assoc-in [id :server :result] server))
      (let [[c] (take 1 client)
            [s] (take 1 server)
            [caddr cport] (get_addr flow id :client)
            [saddr sport] (get_addr flow id :server)
            c2 (-> c
                   (assoc "ip" caddr)
                   (assoc "port" cport))
            s2 (-> s
                   (assoc "ip" saddr)
                   (assoc "port" sport))]
        (send output_agent
              (fn [flow]
                (do
                  (println (json/write-str {:client c2
                                            :server s2}))
                  0)))
        (recur (drop 1 client) (drop 1 server))))))

(defn bytes2str [bytes]
  (apply str (map char bytes)))

(defn read_header [rdr]
  (let [line (.readLine rdr)]
    (if (empty? line)
      (do (printerr "remote socket was closed")
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
          [(-> (take idx line)
               (bytes2str)
               (clojure.string/lower-case)
               (clojure.string/replace #"\." "[dot]"))
           (bytes2str (drop (+ idx 2) line))]))
      (catch IllegalArgumentException e nil))))

(defn get_content_len [flow id peer]
  (Integer. ((get-in flow [id peer :header]) "content-length")))

(defn has_body [flow id peer]
  (contains? (get-in flow [id peer :header]) "content-length"))

(defn is_chunked [flow id peer]
  (= (get-in flow [id peer :header "transfer-encoding"]) "chunked"))

(defn parse_trailer [flow id peer]
  (let [[line new_data] (take_line (get-in flow [id peer :data]))
        length (get-in flow [id peer :length])]
    (if (nil? line)
      [false flow] ;; no lines
      (let [trailer (parse_header line)]
        (condp = trailer
          ;; failed to parse a trailer
          nil [false (assoc-in flow [id :clinet :state] :error)]

          ;; no more trailers
          :end [true
                (-> flow
                    (assoc-in [id peer :length] (+ 2 length))
                    (assoc-in [id peer :data] new_data)
                    (assoc-in [id peer :state] (top peer))
                    (push2result id peer)
                    (pop_result id))]

          ;; have more trailers
          (let [[key value] trailer]
            [true
             (-> flow
                 (assoc-in [id peer :length] (+ length 2 (count line)))
                 (assoc-in [id peer :data] new_data)
                 (assoc-in [id peer :trailer key] value))]))))))

(defn parse_headers [flow id peer]
  (let [[line new_data] (take_line (get-in flow [id peer :data]))]
    (if (nil? line)
      [false flow] ;; no lines
      (let [header (parse_header line)
            length (get-in flow [id peer :length])]
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
                       (assoc-in [id peer :length] (+ length 2))
                       (assoc-in [id peer :data] new_data)
                       (assoc-in [id peer :state] :body)
                       (assoc-in [id peer :remain] (get_content_len flow id peer)))]
                  (catch NumberFormatException e
                    [false (assoc-in flow [id peer :state] :error)]))

                ;; have chunked body
                (is_chunked flow id peer)
                [true (-> flow
                          (assoc-in [id peer :length] (+ length 2))
                          (assoc-in [id peer :state] :chunk-len)
                          (assoc-in [id peer :data] new_data))]

                ;; no body and chunk
                :else [true
                       (-> flow
                           (assoc-in [id peer :length] (+ length 2))
                           (assoc-in [id peer :data] new_data)
                           (assoc-in [id peer :state] (top peer))
                           (push2result id peer)
                           (pop_result id))])

          ;; have more headers
          (let [[key value] header]
            [true
             (-> flow
                 (assoc-in [id peer :length] (+ length 2 (count line)))
                 (assoc-in [id peer :data] new_data)
                 (assoc-in [id peer :header key] value))]))))))

(defn skip_body [flow id peer]
  (let [remain (get-in flow [id peer :remain])
        length (get-in flow [id peer :length])
        data (get-in flow [id peer :data])
        data_len (count data)]
    (if (>= data_len remain)
      [true (-> flow
                (assoc-in [id peer :length] (+ length remain))
                (assoc-in [id peer :data] (drop remain data))
                (assoc-in [id peer :state] (top peer))
                (push2result id peer)
                (pop_result id))]
      [false (-> flow
                 (assoc-in [id peer :length] (+ length data_len))
                 (assoc-in [id peer :remain] (- remain data_len))
                 (assoc-in [id peer :data] '()))])))

(defn parse_chunk_len_line [line]
  (let [idx (.indexOf line ";")]
    (if (= idx -1)
      line
      (take idx line))))

(defn parse_chunk_len [flow id peer]
  (let [[line new_data] (take_line (get-in flow [id peer :data]))
        length (get-in flow [id peer :length])]
    (if (nil? line)
      [false flow]
      (try
        (let [chunk_len (-> line
                      (bytes2str)
                      (parse_chunk_len_line)
                      (Integer/parseInt 16))]
          (if (= chunk_len 0)
            [true
             (-> flow
                 (assoc-in [id peer :length] (+ length 2 (count line)))
                 (assoc-in [id peer :data] new_data)
                 (assoc-in [id peer :state] :trailer))]
            [true
             (-> flow
                 (assoc-in [id peer :length] (+ length 2 (count line)))
                 (assoc-in [id peer :remain] (+ 2 chunk_len))
                 (assoc-in [id peer :data] new_data)
                 (assoc-in [id peer :state] :chunk-body))]))
        (catch NumberFormatException e
          [false (assoc-in flow [id peer :state] :error)])
        (catch IllegalArgumentException e
          [false (assoc-in flow [id peer :state] :error)])))))

(defn skip_chunk_body [flow id peer]
  (let [remain (get-in flow [id peer :remain])
        length (get-in flow [id peer :length])
        data (get-in flow [id peer :data])
        data_len (count data)]
    (if (>= data_len remain)
      [true (-> flow
                (assoc-in [id peer :length] (+ length remain))
                (assoc-in [id peer :data] (drop remain data))
                (assoc-in [id peer :state] :chunk-len))]
      [false (-> flow
                 (assoc-in [id peer :length] (+ length data_len))
                 (assoc-in [id peer :remain] (- remain data_len))
                 (assoc-in [id peer :data] '()))])))

(defn parse [flow0 id buf peer]
  (loop [flow (assoc-in flow0 [id peer :data]
                        (concat (get-in flow0 [id peer :data]) buf))]
    (condp = (get-in flow [id peer :state])
      :method (let [[line new_data] (take_line
                                     (get-in flow [id peer :data]))
                    length (get-in flow [id peer :length])]
                (if (nil? line)
                  flow
                  (let [method (parse_method line)]
                    (if (nil? method)
                      (assoc-in flow [id peer :state] :error)
                      (recur
                       (-> flow
                           (assoc-in [id peer :length] (+ length  2 (count line)))
                           (assoc-in [id peer :method] method)
                           (assoc-in [id peer :data] new_data)
                           (assoc-in [id peer :state] :header)))))))
      :responce (let [[line new_data] (take_line
                                       (get-in flow [id peer :data]))
                      length (get-in flow [id peer :length])]
                  (if (nil? line)
                    flow
                    (let [responce (parse_responce line)]
                      (if (nil? responce)
                        (assoc-in flow [id peer :state] :error)
                        (recur
                         (-> flow
                             (assoc-in [id peer :length] (+ length 2 (count line)))
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
      :chunk-len (let [[is_recur new_flow] (parse_chunk_len flow id peer)]
                   (condp = is_recur
                     false new_flow
                     true (recur new_flow)))
      :chunk-body (let [[is_recur new_flow] (skip_chunk_body flow id peer)]
                    (condp = is_recur
                      false new_flow
                      true (recur new_flow)))
      :trailer (let [[is_recur new_flow] (parse_trailer flow id peer)]
                 (condp = is_recur
                   false new_flow
                   true (recur new_flow)))
      :error flow)))

(defn parse_http [flow header id buf]
  (condp = (header "match")
    "up" (let [from (header "from")
               new_flow (assoc-in flow [id :client :from] from)]
           (parse new_flow id buf :client))
    "down" (let [from (header "from")
                 new_flow (assoc-in flow [id :server :from] from)]
             (parse new_flow id buf :server))))

(defn read_data [rdr header parser session idx]
  (let [len (read-string (header "len"))
        id (dissoc header "event" "from" "match" "len")
        pid (session id)
        buf (make-array Byte/TYPE len)
        rlen (.read rdr buf 0 len)]
    (if (= rlen -1)
      (do (printerr "remote socket was closed")
          (System/exit 0))
      (if (not (nil? pid))
        (let [pagent (nth parser pid)]
          (send pagent parse_http header id buf))))
    [session idx]))

(defn flow_created [rdr header parser session idx]
  (let [id (dissoc header "event")
        pagent (nth parser idx)]
    (send pagent (fn [flow]
                   (assoc flow id {:client {:data '()
                                            :state :method
                                            :result '()
                                            :method nil
                                            :header {}
                                            :trailer {}
                                            :length 0}
                                   :server {:data '()
                                            :state :responce
                                            :result '()
                                            :responce nil
                                            :header {}
                                            :trailer {}
                                            :length 0}})))
    [(assoc session id idx) (mod (inc idx) numpool)]))

(defn flow_destroyed [rdr header parser session idx]
  (let [id (dissoc header "event")
        pid (session id)]
    (if (not (nil? pid))
      (let [pagent (nth parser pid)]
        (send pagent (fn [flow] (-> flow
                                    (pop_result id)
                                    (pop_peer_result id :client)
                                    (pop_peer_result id :server)
                                    (dissoc id))))))
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
        (printerr "error: couldn't read from socket")
        (System/exit 1)))))

(defn uxconnect [uxpath]
  (with-open [sock (AFUNIXSocket/newInstance)]
    (try
      (.connect sock (AFUNIXSocketAddress. (clojure.java.io/file uxpath)))
      (catch AFUNIXSocketException e
        (printerr (str "error: couldn't open \"" uxpath "\""))
        (System/exit 1)))
    (printerr (str "connected to " uxpath))
    (uxreader sock)))

(defn -main
  "I don't do a whole lot ... yet."
  [& args]
  (let [uxpath (nth args 0 "/tmp/sf-tap/tcp/http")]
    (uxconnect uxpath)))
