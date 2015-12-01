#!/bin/bash
#
# Show count of layer 7 record
#
# Requirement: bash version >= 4.0
#
# usage: 
#   socat /tmp/sf-tap/tcp/echo - | bash  echo.sh
#
declare -A network_ident

function show_count7() {
  echo -- current dump result --
  for key in ${!network_ident[@]}; do
    echo "${key} => ${network_ident[$key]}"
  done
  echo -------------------------
}

trap 'show_count7' 2

# network_ident["IDENTFIER"]
# IDENTFIER : XXX.XXX.XXX.XXX:ZZA,YYY.YYY.YYY.YYY:ZZB,hop:ZZC,LEFT_OR_RIGHT
# LEFT_OR_RIGHT : left
#               : right
# 
#   The XYZ or others mean ip or port value.
#
# example header is the below
#   ex. ip1=192.168.191.1,ip2=192.168.191.11,port1=56959,port2=9998,hop=0,l3=ipv4,l4=tcp,event=DATA,from=1,match=none,len=6
# then we use only the tuple construction of ip, port, and hop
# ip1=XXX.XXX.XXX.XXX,ip2=YYY.YYY.YYY.YYY,port1=ZZA,port2=ZZB,hop=ZZC:

## ex. result
#
# ip1=192.168.191.1,ip2=192.168.191.11,port1=61155,port2=9998,hop=0,l3=ipv4,l4=tcp,event=CREATED
# ip1=192.168.191.1,ip2=192.168.191.11,port1=61155,port2=9998,hop=0,l3=ipv4,l4=tcp,event=DATA,from=1,match=none,len=6
# ... snip ...
# ip1=192.168.191.1,ip2=192.168.191.11,port1=61155,port2=9998,hop=0,l3=ipv4,l4=tcp,event=DESTROYED

function header_parse() {
  local header=$1
  local elems=( $(echo $header | tr -s ',' ' ') )
  local ip1=${elems[0]#ip1=}
  local ip2=${elems[1]#ip2=}
  local port1=${elems[2]#port1=}
  local port2=${elems[3]#port2=}
  local hop=${elems[4]#hop=}
  local lr=${elems[8]#from=}

  local status=${elems[7]#event=}
  # echo header: $header
  # echo debug-status: $status
  retval="NEXT"
  for cont_status in "CREATED" "DESTROYED"
  do
    if [ x"$cont_status"=x"$status" ]; then
      retval="NONE"
      break
    fi
  done 
  if [ x"1" = x"$lr" ]; then
    arrow="left"
  else
    arrow="right"
  fi
  key="$ip1:$port1,$ip2,$port2,hop:$hop,$arrow"

  if [ x"CREATED"=x"$status" ]; then
    let network_ident[$key]="0"
    # echo CREATED: network_ident =  ${network_ident[$key]}
  fi
  echo $retval:$key
}

# test_case()
# text="ip1=192.168.191.1,ip2=192.168.191.11,port1=61155,port2=9998,hop=0,l3=ipv4,l4=tcp,event=CREATED"
# text="ip1=192.168.191.1,ip2=192.168.191.11,port1=61155,port2=9998,hop=0,l3=ipv4,l4=tcp,event=DESTROYED"
#

function main() {
  local has_next="NONE"
  while read line
  do
    echo "line:" $line
    val=$(header_parse $line)
    has_next=${val%%:*}
    key=${val#*:}
    if [ x"$has_next"=x"NEXT" ]; then
     read body
     # echo body: $body  # if you want to dump layer 7 data
     # network_ident[$key]=$(echo ${network_ident[$key]} + 1 | bc)
    fi
  done
}

main
