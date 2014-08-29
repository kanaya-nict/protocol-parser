(ns http-proxy.core
  (:gen-class)
  (:import [org.newsclub.net.unix
            AFUNIXSocket AFUNIXSocketAddress AFUNIXSocketException]))

(use '[clojure.string :only (split)])

(defn printerr [& args]
  (binding [*out* *err*]
    (apply println args)))

(defn bytes2str [bytes]
  (apply str (map char bytes)))

(defn read_header [rdr]
  (let [line (.readLine rdr)]
    (if (empty? line)
      (do (printerr "remote socket was closed")
          (System/exit 0))
      [(apply hash-map (flatten (map #(split % #"=") (split line #","))))
       line])))

(defn flow_created [session header line wtr_lb]
  (let [id (dissoc header "event")]
    (.writeBytes wtr_lb line)
    (.writeByte wtr_lb (int \newline))
    (assoc session id {:client {:data '()
                                :length 0
                                :method nil
                                :header {}
                                :state :method}
                       :server {:data '()
                                :length 0
                                :responce nil
                                :header {}
                                :state :responce}})))

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

(defn parse_headers [session id peer]
  (let [[line new_data] (take_line (get-in session [id peer :data]))]
    (if (nil? line)
      [false session] ;; no lines
      (let [header (parse_header line)
            length (get-in session [id peer :length])]
        (condp = header
          ;; failed to parse a header
          nil [false (assoc-in session [id :clinet :state] :error)]

          ;; no more headers
          :end [true
                (-> session
                    (assoc-in [id peer :length] (+ length 2))
                    (assoc-in [id peer :data] new_data)
                    (assoc-in [id peer :state] :body))]

          ;; have more headers
          (let [[key value] header]
            [true
             (-> session
                 (assoc-in [id peer :length] (+ length 2 (count line)))
                 (assoc-in [id peer :data] new_data)
                 (assoc-in [id peer :header key] value))]))))))

(defn parse [session0 header hdline id buf peer wtr_lb]
  (if (= :data (get-in session0 [id peer :state]))
    (do
      (.writeBytes wtr_lb hdline)
      (.writeByte wtr_lb (int \newline))
      (.write wtr_lb 0 buf (count buf))
      session0)
    (loop [session (assoc-in session0 [id peer :data]
                             (concat (get-in session0 [id peer :data]) buf))]
      (condp = (get-in session [id peer :state])
        :method (let [[line new_data] (take_line
                                       (get-in session [id peer :data]))
                      length (get-in session [id peer :length])]
                  (if (nil? line)
                    session
                    (let [method (parse_method line)]
                      (if (nil? method)
                        (assoc-in session [id peer :state] :error)
                        (recur
                         (-> session
                             (assoc-in [id peer :length] (+ length  2 (count line)))
                             (assoc-in [id peer :method] method)
                             (assoc-in [id peer :data] new_data)
                             (assoc-in [id peer :state] :header)))))))

        :responce (let [[line new_data] (take_line
                                         (get-in session [id peer :data]))
                        length (get-in session [id peer :length])]
                    (if (nil? line)
                      session
                      (let [responce (parse_responce line)]
                        (if (nil? responce)
                          (assoc-in session [id peer :state] :error)
                          (recur
                           (-> session
                               (assoc-in [id peer :length] (+ length 2 (count line)))
                               (assoc-in [id peer :responce] responce)
                               (assoc-in [id peer :data] new_data)
                               (assoc-in [id peer :state] :header)))))))

        :header (let [[is_recur new_session] (parse_headers session id peer)]
                  (condp = is_recur
                    false new_session
                    true (recur new_session)))

        :data (let [data (byte-array (map byte (get-in session [id peer :data])))
                    length (get-in session [id peer :length])
                    dlen (count data)
                    header2 (assoc header "len" dlen)
                    hdline2 (clojure.string/join "," (for [[k v] (seq header2)] (str k "=" v)))]
                (.writeBytes wtr_lb hdline2)
                (.writeByte wtr_lb (int \newline))
                (.write wtr_lb 0 data dlen)
                (-> session
                    (assoc-in [id peer :length] (+ length dlen))
                    (assoc-in [id peer :data] '())))
        :error session))))

(defn parse_http [session header line id buf wtr_lb]
  (condp = (header "match")
    "up" (let [from (header "from")
               session2 (assoc-in session [id :client :from] from)]
           (parse session2 header line id buf :client wtr_lb))
    "down" (let [from (header "from")
                 session2 (assoc-in session [id :server :from] from)]
             (parse session2 header line id buf :server wtr_lb))))

(defn read_data [session header line rdr_proxy wtr_lb]
  (try
    (let [len (-> header
                  (get "len")
                  (Integer/parseInt))
          id (dissoc header "event" "from" "match" "len")
          buf (make-array Byte/TYPE len)
          rlen (.read rdr_proxy buf 0 len)]
      (cond
       (= rlen -1)
       (do (printerr "remote socket was closed")
           (System/exit 0))

       (not= rlen len)
       (do (printerr "couldn't read required bytes: length = " len)
           (System/exit 0))

       :else
       (parse_http session header line id buf wtr_lb)))
    (catch NumberFormatException e
      (printerr "error: data header has invalid length: len = "
                (str (header "len")))
      (System/exit 1))))

(defn flow_destroyed [session header line wtr_lb]
  (let [id (dissoc header "event")]
    ;; TODO: output JSON
    (.writeBytes wtr_lb line)
    (.writeByte wtr_lb (int \newline))
    (dissoc session id)))

(defn uxreader [sock_proxy sock_lb]
  (with-open [rdr_proxy (java.io.DataInputStream. (.getInputStream sock_proxy))
              wtr_lb (java.io.DataOutputStream. (.getOutputStream sock_lb))]
    (try
      (loop [session {}]
        (let [[header line] (read_header rdr_proxy)
              new_session (condp = (header "event")
                            "CREATED" (flow_created session header line wtr_lb)
                            "DATA" (read_data session header line rdr_proxy wtr_lb)
                            "DESTROYED" (flow_destroyed session header line wtr_lb))]
          (recur new_session)))
      (catch java.io.IOException e
        (printerr "error: couldn't read from socket")
        (System/exit 1)))))

(defn uxconnect [uxproxy uxlb]
  (with-open [sock_proxy (AFUNIXSocket/newInstance)
              sock_lb (AFUNIXSocket/newInstance)]
    (try
      (.connect sock_proxy (AFUNIXSocketAddress. (clojure.java.io/file uxproxy)))
      (.connect sock_lb (AFUNIXSocketAddress. (clojure.java.io/file uxlb)))
      (catch AFUNIXSocketException e
        (printerr (str e))
        (System/exit 1)))
    (printerr (str "connected to " uxproxy))
    (printerr (str "connected to " uxlb))
    (uxreader sock_proxy sock_lb)))

(defn -main
  "Redirector of HTTP Proxy for SF-TAP"
  [& args]
  (let [uxproxy (nth args 0 "/tmp/sf-tap/tcp/http_proxy")
        uxlb (nth args 1 "/tmp/sf-tap/loopback7")]
    (uxconnect uxproxy uxlb)))
