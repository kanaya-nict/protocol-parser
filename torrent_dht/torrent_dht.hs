module Main where
import Network.Socket
import System.IO
import Data.List.Split
import qualified Data.ByteString as ByteString
import qualified Data.List as List
import qualified Data.Map as Map
import qualified Data.AttoBencode as Bencode

open_ux :: String -> IO Handle
open_ux path =
  do
    sock <- socket AF_UNIX Stream 0
    connect sock (SockAddrUnix path)
    h <- socketToHandle sock ReadMode
    hSetBuffering h (BlockBuffering Nothing)
    return h

print_bencode :: Maybe Bencode.BValue -> IO ()
print_bencode bc =
  case bc of
   Nothing  -> hPutStrLn stderr "parse error"
   Just val -> putStrLn $ show val

print_data :: System.IO.Handle -> String -> IO ()
print_data h line =
  do
    putStrLn line
    bytes <- ByteString.hGet h len
    print_bencode $ Bencode.decode bytes
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
