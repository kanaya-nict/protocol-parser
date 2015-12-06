# Handler for HTTP Proxy in Julia Language

import JSON

@enum http_state HTTP_INIT HTTP_HEADER HTTP_BODY

type http_method
    method::ASCIIString
    url::ASCIIString
    ver::ASCIIString
end

type http_response
    ver::ASCIIString
    code::ASCIIString
    comment::ASCIIString
end

type http_flow
    up::Vector{UInt8}
    down::Vector{UInt8}
    up_state::http_state
    down_state::http_state
    method::http_method
    response::http_response
    client_header::Dict{ASCIIString, ASCIIString}
    server_header::Dict{ASCIIString, ASCIIString}
    client_time::Float64
    server_time::Float64
end

type http_proxy
    sock_proxy::Base.PipeEndpoint
    sock_lb7::Base.PipeEndpoint
    flows::Dict

    function http_proxy(path, l7path)
        new(connect(path), connect(l7path), Dict())
    end
end

function print_json(hdic, flow::http_flow)
    if flow.up_state != HTTP_BODY || flow.down_state != HTTP_BODY
        return
    end

    sip = ""
    cip = ""
    sport = ""
    cport = ""

    if hdic["match"] == "up" && hdic["from"] == "1" ||
       hdic["match"] == "down" && hdic["from"] == "2"
        sip   = hdic["ip2"]
        cip   = hdic["ip1"]
        sport = parse(Int, hdic["port2"])
        cport = parse(Int, hdic["port1"])
    else
        sip   = hdic["ip1"]
        cip   = hdic["ip2"]
        sport = parse(Int, hdic["port1"])
        cport = parse(Int, hdic["port2"])
    end

    d = Dict()
    d["server"] = Dict()
    d["server"]["ip"]       = sip
    d["server"]["port"]     = sport
    d["server"]["response"] = flow.response
    d["server"]["response"] = flow.server_time

    if ! isempty(length(flow.server_header))
        d["server"]["header"] = flow.server_header
    end

    d["client"] = Dict()
    d["client"]["ip"]     = cip
    d["client"]["port"]   = cport
    d["client"]["method"] = flow.method
    d["client"]["time"]   = flow.client_time

    if ! isempty(flow.client_header)
        d["client"]["header"] = flow.client_header
    end

    println(JSON.json(d))
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

    id = (hdic["ip1"], hdic["ip2"],
          parse(UInt16, hdic["port1"]), parse(UInt16, hdic["port2"]),
          parse(UInt8, hdic["hop"]))

    return hdic, id
end

function removeline(data::Vector{UInt8})
    idx = findfirst(data, UInt8('\n'))
    if idx > 0
        return ascii(splice!(data, 1:idx))
    end

    return false
end

function parse_method(data::ASCIIString)
    sp = split(rstrip(data), [' '], limit=3)
    try http_method(sp[1], sp[2], sp[3]) catch http_method("", "", "") end
end

function parse_response(data::ASCIIString)
    sp = split(rstrip(data), [' '], limit=3)
    try http_response(sp[1], sp[2], sp[3]) catch http_response("", "", "") end
end

function parse_header(data::ASCIIString)
    sp = split(rstrip(data), [':'], limit=2)
    try (sp[1], lstrip(sp[2])) catch ("", "") end
end

function gen_sftap_header(d, len)
    string("ip1=", d["ip1"], ",ip2=", d["ip2"],
           ",port1=", d["port1"], ",port2=", d["port2"],
           ",hop=", d["hop"], ",l3=", d["l3"], ",l4=", d["l4"],
           ",event=", d["event"], ",from=", d["from"], ",len=", len, "\n")
end

function in_client_data(proxy::http_proxy, header, hdic, id, bytes)
    if proxy.flows[id].up_state == HTTP_BODY
        write(proxy.sock_lb7, header)
        write(proxy.sock_lb7, bytes)
        return
    else
        append!(proxy.flows[id].up, bytes)
    end

    while true
        if proxy.flows[id].up_state == HTTP_BODY
            # write to loopback7
            len = length(proxy.flows[id].up)
            if len > 0
                h = gen_sftap_header(hdic, len)
                write(proxy.sock_lb7, h)
                write(proxy.sock_lb7, proxy.flows[id].up)
            end

            return
        else
            if proxy.flows[id].up_state == HTTP_INIT
                line = removeline(proxy.flows[id].up)
                if line == false
                    return
                else
                    method = parse_method(line)
                    proxy.flows[id].client_time = parse(Float64, hdic["time"])
                    proxy.flows[id].up_state    = HTTP_HEADER
                    proxy.flows[id].method      = method
                end
            else # HTTP_HEADER
                line = removeline(proxy.flows[id].up)
                if line == false
                    return
                elseif line == "\r\n"
                    proxy.flows[id].up_state = HTTP_BODY
                    print_json(hdic, proxy.flows[id])
                else
                    h = parse_header(line)
                    proxy.flows[id].client_header[h[1]] = h[2]
                end
            end
        end
    end
end

function in_server_data(proxy::http_proxy, header, hdic, id, bytes)
    if proxy.flows[id].down_state == HTTP_BODY
        write(proxy.sock_lb7, header)
        write(proxy.sock_lb7, bytes)
        return
    else
        append!(proxy.flows[id].down, bytes)
    end

    while true
        if proxy.flows[id].down_state == HTTP_BODY
            # write to loopback7
            len = length(proxy.flows[id].up)
            if len > 0
                h = gen_sftap_header(hdic, len)
                write(proxy.sock_lb7, h)
                write(proxy.sock_lb7, proxy.flows[id].up)
            end

            return
        else
            if proxy.flows[id].down_state == HTTP_INIT
                line = removeline(proxy.flows[id].down)
                if line == false
                    return
                else
                    resp = parse_response(line)
                    proxy.flows[id].server_time = parse(Float64, hdic["time"])
                    proxy.flows[id].down_state  = HTTP_HEADER
                    proxy.flows[id].response    = resp
                end
            else # HTTP_HEADER                
                line = removeline(proxy.flows[id].down)
                if line == false
                    return
                elseif line == "\r\n"
                    proxy.flows[id].down_state = HTTP_BODY
                    print_json(hdic, proxy.flows[id])
                else
                    h = parse_header(line)
                    proxy.flows[id].server_header[h[1]] = h[2]
                end
            end
        end
    end
end

function in_data(proxy::http_proxy, header, hdic, id, bytes)
    if hdic["match"] == "up"
        in_client_data(proxy, header, hdic, id, bytes)
    else # down
        in_server_data(proxy, header, hdic, id, bytes)
    end
end

function run(proxy::http_proxy)
    while ! eof(proxy.sock_proxy)
        header = readline(proxy.sock_proxy)
        hdic, id = header2dic(header)

        if hdic["event"] == "DATA"
            bytes = readbytes(proxy.sock_proxy, parse(Int, hdic["len"]))
            if haskey(proxy.flows, id)
                in_data(proxy, header, hdic, id, bytes)
            end
        elseif hdic["event"] == "CREATED"
            proxy.flows[id] = http_flow(Vector{UInt8}(), Vector{UInt8}(),
                                        HTTP_INIT, HTTP_INIT,
                                        http_method("", "", ""),
                                        http_response("", "", ""),
                                        Dict{ASCIIString, ASCIIString}(),
                                        Dict{ASCIIString, ASCIIString}(),
                                        0.0, 0.0)
            write(proxy.sock_lb7, header)
        else # DESTROYED
            delete!(proxy.flows, id)
            write(proxy.sock_lb7, header)
        end
    end
end

proxy = http_proxy("/tmp/sf-tap/tcp/http_proxy", "/tmp/sf-tap/loopback7")
run(proxy)
