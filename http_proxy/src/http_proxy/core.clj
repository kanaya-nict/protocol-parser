(ns http-proxy.core
  (:gen-class)
  (:import [org.newsclub.net.unix
            AFUNIXSocket AFUNIXSocketAddress AFUNIXSocketException]))

(use '[clojure.string :only (split)])

(defn printerr [& args]
  (binding [*out* *err*]
    (apply println args)))

(defn read_header [rdr]
  (let [line (.readLine rdr)]
    (if (empty? line)
      (do (printerr "remote socket was closed")
          (System/exit 0))
      [(apply hash-map (flatten (map #(split % #"=") (split line #","))))
       line])))

(defn flow_created [session header line rdr_lb]
  (let [id (dissoc header "event")]
    ;; TODO: write to loopback
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

(defn read_data [session header line rdr_proxy rdr_lb] session)

(defn flow_destroyed [session header line rdr_lb]
  (let [id (dissoc header "event")]
    ;; TODO: write to loopback
    ;;       output to JSON
    (dissoc session id)))

(defn uxreader [sock_proxy sock_lb]
  (with-open [rdr_proxy (java.io.DataInputStream. (.getInputStream sock_proxy))
              rdr_lb (java.io.DataOutputStream. (.getOutputStream sock_lb))]
    (try
      (loop [session {}]
        (let [[header line] (read_header rdr_proxy)
              new_session (condp = (header "event")
                            "CREATED" (flow_created session header line rdr_lb)
                            "DATA" (read_data session header line rdr_proxy rdr_lb)
                            "DESTROYED" (flow_destroyed session header line rdr_lb))]
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
