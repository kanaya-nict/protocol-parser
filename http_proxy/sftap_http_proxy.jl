# UNDER IMPLEMENTING!
#
# *** THIS CODE DOES NOT WORK CURRENTRY ***
#
# Handler for HTTP Proxy in Julia Language

@enum http_state HTTP_INIT HTTP_HEADER HTTP_BODY

type http_method
    method::ASCIIString
    url::ASCIIString
    ver::ASCIIString
end

type http_response
    code::ASCIIString
    ver::ASCIIString
    comment::ASCIIString
end

type http_flow
    up::Vector{UInt8}
    down::Vector{UInt8}
    up_state::http_state
    down_state::http_state
    method::http_method
end

type http_proxy
    sock_proxy::Base.PipeEndpoint
    sock_lb7::Base.PipeEndpoint
    flows::Dict

    function http_proxy(path, l7path)
        new(connect(path), connect(l7path), Dict())
    end
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
    m = rstrip(data)
end

function parse_response(data::ASCIIString)
    r = rstrip(data)
end

function parse_header(data::ASCIIString)
    h = rstrip(data)
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
                    print(header)
                    println(method)
                    proxy.flows[id].up_state = HTTP_HEADER
                end
            else # HTTP_HEADER
                line = removeline(proxy.flows[id].up)
                if line == false
                    return
                elseif line == "\r\n"
                    proxy.flows[id].up_state = HTTP_BODY
                else
                    h = parse_header(line)
                    println(h)
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
                    print(header)
                    println(resp)
                    proxy.flows[id].down_state = HTTP_HEADER
                end
            else # HTTP_HEADER                
                line = removeline(proxy.flows[id].down)
                if line == false
                    return
                elseif line == "\r\n"
                    proxy.flows[id].down_state = HTTP_BODY
                else
                    h = parse_header(line)
                    println(h)
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
                                        http_method("", "", ""))
            write(proxy.sock_lb7, header)
        else # DESTROYED
            delete!(proxy.flows, id)
            write(proxy.sock_lb7, header)
        end
    end
end

proxy = http_proxy("/tmp/sf-tap/tcp/http_proxy", "/tmp/sf-tap/loopback7")
run(proxy)
