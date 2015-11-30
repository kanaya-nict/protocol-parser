# UNDER IMPLEMENTING!
#
# *** THIS CODE DOES NOT WORK CURRENTRY ***
#
# Handler for HTTP Proxy in Julia Language

type flow_id
    ip1::ASCIIString
    ip2::ASCIIString
    port1::UInt16
    port2::UInt16
    hop::UInt8
end

@enum http_state HTTP_INIT HTTP_HEADER HTTP_BODY

type flow
    up
    down
    up_state::http_state
    down_state::http_state
end

function header2dic(header::ASCIIString)
    h = rstrip(header)
    sp = split(h, [',', '='])

    hdic = Dict{ASCIIString, ASCIIString}()

    i = 1
    while i <= length(sp)
        hdic[sp[i]] = sp[i + 1]
        i += 2
    end

    id = flow_id(hdic["ip1"], hdic["ip2"],
                 parse(UInt16, hdic["port1"]), parse(UInt16, hdic["port2"]),
                 parse(UInt8, hdic["hop"]))

    return hdic, id
end

function parse_method(data::Vector{UInt8})
end

function parse_responce(data::Vector{UInt8})
end

function in_data(hstr, hdic, flows::flow, id::flow_id, lp7)
    if hdic["match"] == "up"
        if flows[id].up_state == HTTP_BODY
        else
            append!(flows[id].up, bytes)
        end
    else # down
        if flows[id].down_state == HTTP_BODY
        else
            append!(flows[id].down, bytes)
        end
    end
end

function http_proxy(path, lpath)
    c   = connect(path)
    lp7 = connect(lpath)

    flows = Dict{flow_id, flow}()

    while ! eof(c)
        header = readline(c)
        hdic, id = header2dic(header)

        println(hdic)
        println(id)

        if hdic["event"] == "DATA"
            bytes = readbytes(c, parse(Int, hdic["len"]))
            if haskey(flows, id)
                in_data(header, hdic, flows, id, lp7)
            end
        elseif hdic["event"] == "CREATED"
            flows[id] = flow(Vector{UInt8}(), Vector{UInt8}(),
                             HTTP_INIT, HTTP_INIT)
        else # DESTROYED
            delete!(flows, id)
        end
    end
end

http_proxy("/tmp/sf-tap/tcp/http", "/tmp/sf-tap/loopback7")
