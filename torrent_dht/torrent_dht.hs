module Main where

import Network.Socket
import System.IO
import Data.List.Split
import qualified Data.ByteString as B
import qualified Data.ByteString.Lazy as L
import qualified Data.ByteString.Char8 as C
import qualified Data.List as List
import qualified Data.Map as Map
import qualified Data.AttoBencode as Bencode
import qualified Text.JSON as JSON
import qualified Data.Binary.Get as Bin
import qualified Data.ByteString.Base64 as B64

open_ux :: String -> IO Handle
open_ux path =
  do
    sock <- socket AF_UNIX Stream 0
    connect sock (SockAddrUnix path)
    h <- socketToHandle sock ReadMode
    hSetBuffering h (BlockBuffering Nothing)
    return h

print_dict :: [(B.ByteString, Bencode.BValue)] -> IO ()
print_dict x =
  do
    putStr "{"
    print_dict_elm1 x
    putStr "}"

print_dict_elm1 :: [(B.ByteString, Bencode.BValue)] -> IO ()
print_dict_elm1 (x:xs) =
  do
    print_dict_kv x
    print_dict_elm xs
print_dict_elm1 _ = putStr ""

print_dict_elm :: [(B.ByteString, Bencode.BValue)] -> IO ()
print_dict_elm (x:xs) =
  do
    putStr ","
    print_dict_kv x
    print_dict_elm xs
print_dict_elm _ = putStr ""

print_ip :: B.ByteString -> IO ()
print_ip bytes =
  do
    putStr "\"ip\":\""
    addr <- inet_ntoa . Bin.runGet Bin.getWord32host . L.fromStrict $ bytes
    putStr addr
    putStr "\""

print_node_list1 :: B.ByteString -> IO ()
print_node_list1 bytes =
  if len >= 6 then
    do
      putStr "[\""
      addr <- inet_ntoa . Bin.runGet Bin.getWord32host . L.fromStrict . B.take 4 $ bytes
      putStr addr
      putStr "\","
      putStr . show . Bin.runGet Bin.getWord16be . L.fromStrict . B.take 2 . B.drop 4 $ bytes
      putStr "]"
      print_node_list $ B.drop 6 bytes
  else
    putStr ""
  where
    len = B.length bytes

print_node_list :: B.ByteString -> IO ()
print_node_list bytes =
  if len >= 6 then
    do
      putStr ",[\""
      addr <- inet_ntoa . Bin.runGet Bin.getWord32host . L.fromStrict . B.take 4 $ bytes
      putStr addr
      putStr "\","
      putStr $ show . Bin.runGet Bin.getWord16be . L.fromStrict . B.take 2 . B.drop 4 $ bytes
      putStr "]"
      print_node_list $ B.drop 6 bytes
  else
    putStr ""
  where
    len = B.length bytes

print_nodes_bin :: B.ByteString -> IO ()
print_nodes_bin bytes =
  do
    putStr "\"nodes\":["
    print_node_list1 bytes
    putStr "]"

print_list_elm :: Bencode.BValue -> (B.ByteString -> IO ()) -> IO ()
print_list_elm x f =
  case x of
   Bencode.BDict dict  -> print_dict $ Map.toList dict
   Bencode.BList list  -> print_list list f
   Bencode.BString str -> f str
   _ -> putStr $ show x

print_list_elm1 :: [Bencode.BValue] -> (B.ByteString -> IO ()) -> IO ()
print_list_elm1 (x:xs) f =
  do
    print_list_elm x f
    print_list_elm2 xs f
print_list_elm1 _ _ = putStr ""

print_list_elm2 :: [Bencode.BValue] -> (B.ByteString -> IO ()) -> IO ()
print_list_elm2 (x:xs) f =
  do
    putStr ","
    print_list_elm x f
    print_list_elm2 xs f
print_list_elm2 _ _ = putStr ""

print_list :: [Bencode.BValue] -> (B.ByteString -> IO ())  -> IO ()
print_list x f =
  do
    putStr "["
    print_list_elm1 x f
    putStr "]"

print_dict_kv :: (B.ByteString, Bencode.BValue) -> IO ()
print_dict_kv (k, v) =
  case v of
   Bencode.BDict dict ->
     do
       print_str k
       putStr ":"
       print_dict $ Map.toList dict
   Bencode.BString val ->
     case key of
      "ip"    -> print_ip val
      "nodes" -> print_nodes_bin val
      "y" ->
        do
          print_str k
          putStr ":"
          print_str val
      _ ->
        do
          print_str k
          putStr ":"
          print_base64 val
     where
       key = C.unpack k
   Bencode.BList list ->
     case key of
      "nodes" ->
        do
          print_str k
          putStr ":"
          print_list list print_str
      _ ->
        do
          print_str k
          putStr ":"
          print_list list print_base64
     where
       key = C.unpack k
   Bencode.BInt val ->
     do
       print_str k
       putStr ":"
       putStr $ show val

print_str :: B.ByteString -> IO ()
print_str x = putStr . JSON.encode . JSON.toJSString . C.unpack $ x

print_base64 :: B.ByteString -> IO ()
print_base64 x = putStr . show . B64.encode $ x

print_bencode :: Maybe Bencode.BValue -> IO ()
print_bencode bc =
  case bc of
   Nothing  -> hPutStrLn stderr "parse error"
   Just (Bencode.BDict dict) -> print_dict $ Map.toList dict
   _ -> hPutStrLn stderr "invalid data"

print_ip_port :: String -> Maybe String -> Maybe String -> IO ()
print_ip_port x (Just ip) (Just port) =
  do
    putStr "\""
    putStr x
    putStr "\":{"
    putStr "\"ip\":\""
    putStr ip
    putStr "\",\"port\":"
    putStr port
    putStr "}"
print_ip_port x Nothing (Just port) =
  do
    putStr "\""
    putStr x
    putStr "\":{\"port\":"
    putStr port
    putStr "}"
print_ip_port x (Just ip) Nothing =
  do
    putStr "\""
    putStr x
    putStr "\":{\"ip\":\""
    putStr ip
    putStr "\"}"
print_ip_port x _ _ =
  do
    putStr "\""
    putStr x
    putStr "\":{}"

print_time :: Maybe String -> IO ()
print_time (Just time) =
  do
    putStr "\"time\":"
    putStr time
print_time _ =
  putStr "\"time\":0"

print_header :: Map.Map String String -> IO ()
print_header m =
  do
    print_ip_port "src" ip_src port_src
    putStr ","
    print_ip_port "dst" ip_dst port_dst
    putStr ","
    print_time time
  where
    from = Map.lookup "from" m
    ip_src = case from of
              Nothing  -> Nothing
              Just "1" -> Map.lookup "ip1" m
              Just "2" -> Map.lookup "ip2" m
    ip_dst = case from of
              Nothing  -> Nothing
              Just "1" -> Map.lookup "ip2" m
              Just "2" -> Map.lookup "ip1" m
    port_src = case from of
                Nothing  -> Nothing
                Just "1" -> Map.lookup "port1" m
                Just "2" -> Map.lookup "port2" m
    port_dst = case from of
                Nothing  -> Nothing
                Just "1" -> Map.lookup "port2" m
                Just "2" -> Map.lookup "port1" m
    time = Map.lookup "time" m

print_data :: System.IO.Handle -> String -> IO ()
print_data h line =
  do
    bytes <- B.hGet h len
    putStr "{"
    print_header m
    putStr ",\"bencode\":"
    print_bencode $ Bencode.decode bytes
    putStrLn "}"
  where
    m = header_str_to_map line
    len = get_data_len m

pair_list :: [a] -> [(a,a)] -> [(a,a)]
pair_list (x:y:xs) ret = pair_list xs ((x,y):ret)
pair_list _ ret = ret

header_str_to_map :: String -> Map.Map String String
header_str_to_map str =
  Map.fromList $ pair_list sp []
  where
    sp = splitWhen (\c -> c == '=' || c == ',') str

get_data_len :: Map.Map String String -> Int
get_data_len m =
  case val of
   Nothing  -> error "invalid header"
   Just str -> read $ str :: Int
  where val = Map.lookup "len" m

main_loop :: System.IO.Handle -> IO ()
main_loop h =
  do
    str <- hGetLine h
    print_data h str
    main_loop h

main =
  do
    h <- open_ux "/tmp/sf-tap/udp/torrent_dht"
    main_loop h
