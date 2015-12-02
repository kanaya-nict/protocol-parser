#!/bin/bash
#
# Show count of layer 7 record
#
# Requirement: bash version >= 4.0, base64
#
# usage:
#   socat /tmp/sf-tap/tcp/echo - | bash echo.sh
#
# ip1=192.168.191.1,ip2=192.168.191.11,port1=61155,port2=9998,hop=0,l3=ipv4,l4=tcp,event=CREATED
# ip1=192.168.191.1,ip2=192.168.191.11,port1=61155,port2=9998,hop=0,l3=ipv4,l4=tcp,event=DATA,from=1,match=none,len=7
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

  local status=${elems[7]#event=}
  retval="NEXT"
  for cont_status in "CREATED" "DESTROYED"
  do
    if [ x"$cont_status" = x"$status" ]; then
      retval="NONE"
      break
    fi
  done 
  if [ x"$retval" != x"NONE" ]; then
    local lr=${elems[8]#from=}
    local len="${elems[10]#len=}"
    retval="${retval}=${len}"
  fi 
  if [ x"1" = x"$lr" ]; then
    arrow="left"
  else
    arrow="right"
  fi
  local key="$ip1,$port1,$ip2,$port2,$hop,$status,$arrow"

  echo "$retval:$key"
}

# test_case()
# text="ip1=192.168.191.1,ip2=192.168.191.11,port1=61155,port2=9998,hop=0,l3=ipv4,l4=tcp,event=CREATED"
# text="ip1=192.168.191.1,ip2=192.168.191.11,port1=61155,port2=9998,hop=0,l3=ipv4,l4=tcp,event=DESTROYED"
#

function main() {
  local has_next_status="NONE"
  while read line
  do
    val=$(header_parse $line)
    local retval=${val%%:*}
    has_next_status=${retval%%=*}
    local key=${val#*:}
    local body=""
    if [ x"$has_next_status" = x"NEXT" ]; then
      local len=${retval#*=}
      body=$(head -c $len | base64)
      comma_or_not=","
      body_header_data="\"body\":\"$body\",\n  \"len\":\"$len\""
    fi

    local elems=( $(echo $key | tr -s ',' ' ') )
    local ip1=${elems[0]}
    local ip2=${elems[1]}
    local port1=${elems[2]}
    local port2=${elems[3]}
    local hop=${elems[4]}
    local event=${elems[5]}
    local arrow=${elems[6]}

    cat<<JSON_TEMPLATE
{
 "ip1":"$ip1",
 "port1":"$port1",
 "ip2":"$ip2",
 "port2":"$port2",
 "hop":"$hop",
 "event":"$event",
 "arrow":"$arrow" $comma_or_not
 $( echo -e $body_header_data  )
}
JSON_TEMPLATE
  done
}

main
