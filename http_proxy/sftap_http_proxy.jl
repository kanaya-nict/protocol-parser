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

@enum http_state HTTP_HEADER HTTP_BODY

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

function read_line_uint8v(vec)
    line = Vector{Uint8}()
    for v in vec
        for bytes in v
            findfirst(bytes, Uint8('\n'))
        end
    end
end

function http_proxy(path, lpath)
    c   = connect(path)
    lpc = connect(lpath)

    flows = Dict{flow_id, flow}()

    while ! eof(c)
        header = readline(c)
        hdic, id = header2dic(header)

        println(hdic)
        println(id)

        if hdic["event"] == "DATA"
            bytes = readbytes(c, parse(Int, hdic["len"]))
            if haskey(flows, id)
                if hdic["event"] == "up"
                    push!(flows[id].up, bytes)
                else
                    push!(flows[id].down, bytes)
                end
            end
        elseif hdic["event"] == "CREATED"
            flows[id] = flow([], [], HTTP_HEADER, HTTP_HEADER)
        else # DESTROYED
            delete!(flows, id)
        end
    end
end

http_proxy("/tmp/sf-tap/tcp/http", "/tmp/sf-tap/loopback7")
