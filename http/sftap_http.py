import socket

uxpath = '/tmp/sf-tap/tcp/http'

class http_parser:
    def __init__(self, is_client = True):
        self.__METHOD  = 0
        self.__RESP    = 1
        self.__HEADER  = 2
        self.__BODY    = 3
        self.__TRAILER = 4
        self.__CHUNK_LEN  = 5
        self.__CHUNK_BODY = 6
        self.__CHUNK_END  = 7

        self._is_client = is_client

        if is_client:
            self._state = self.__METHOD
        else:
            self._state = self.__RESP

        self._data = []
        self.result = []

        self._ip       = b''
        self._port     = b''
        self._method   = {}
        self._response = {}
        self._resp     = {}
        self._header   = {}
        self._trailer  = {}
        self._length   = 0
        self._remain   = 0

        self.__is_error = False

    def in_data(self, data, header):
        if self.__is_error:
            return

        if self._ip == b'' or self._port == b'':
            if header[b'from'] == b'1':
                self._ip   = header[b'ip1']
                self._port = header[b'port1']
            elif header[b'from'] == b'2':
                self._ip   = header[b'ip2']
                self._port = header[b'port2']

        self._data.append(data)
        self._parse(header)

    def _push_data(self):
        result = {}

        if self._is_client:
            result['method'] = self._method
        else:
            result['response'] = self._response

        result['header']  = self._header
        result['trailer'] = self._trailer
        result['ip']      = self._ip
        result['port']    = self._port

        print(result)

        self.result.append(result)

        self._method   = {}
        self._response = {}
        self._resp     = {}
        self._header   = {}
        self._trailer  = {}
        self._length   = 0
        self._remain   = 0

    def _parse(self, header):
        while True:
            if self._state == self.__METHOD:
                if not self._parse_method():
                    break
            elif self._state == self.__RESP:
                if not self._parse_response():
                    break
            elif self._state == self.__HEADER:
                if not self._parse_header():
                    break
            elif self._state == self.__BODY:
                self._skip_body()
                if self._remain > 0:
                    break
            elif self._state == self.__CHUNK_LEN:
                if not self._parse_chunk_len():
                    break
            elif self._state == self.__CHUNK_BODY:
                self._skip_body()
                if self._remain > 0:
                    break

                self._state = self.__CHUNK_LEN
            elif self._state == self.__CHUNK_END:
                self._skip_body()
                if self._remain > 0:
                    break

                self._state = self.__TRAILER
            else:
                break

    def _parse_chunk_len(self):
        (result, line) = self._read_line()

        if result:
            self._remain = int(line.split(b';')[0], 16) + 2
            self._state = self.__CHUNK_BODY

            if self._remain == 2:
                self._state = self.__CHUNK_END
            return True
        else:
            return False

    def _parse_trailer(self):
        (result, line) = self._read_line()

        if result:
            if len(line) == 0:
                if self._is_client:
                    self._state = self.__METHOD
                else:
                    self._state = self.__RESP
            else:
                sp = line.split(b': ')

                self._trailer[sp[0]] = sp[1]
            return True
        else:
            return False

    def _parse_method(self):
        (result, line) = self._read_line()

        if result:
            sp = line.split(b' ')

            self._method[b'method'] = sp[0]
            self._method[b'uri']    = sp[1]
            self._method[b'ver']    = sp[2]

            self._state = self.__HEADER
            return True
        else:
            return False

    def _parse_response(self):
        (result, line) = self._read_line()

        if result:
            sp = line.split(b' ')

            self._response[b'ver']  = sp[0]
            self._response[b'code'] = sp[1]
            self._response[b'msg']  = b' '.join(sp[2:])

            self._state = self.__HEADER
            return True
        else:
            return False

    def _parse_header(self):
        (result, line) = self._read_line()

        if result:
            if line == b'':
                if b'content-length' in self._header:
                    self._remain = int(self._header[b'content-length'])

                    if self._remain > 0:
                        self._state = self.__BODY
                    elif (b'transfer-encoding' in self._header and
                          self._header[b'transfer-encoding'].lower() == b'chunked'):
                        self._state = self.__CHUNK_LEN
                    elif self._is_client:
                        self._push_data()
                        self._state = self.__METHOD
                    else:
                        self._push_data()
                        self._state = self.__RESP
                elif (b'transfer-encoding' in self._header and
                      self._header[b'transfer-encoding'].lower() == b'chunked'):

                    self._state = self.__CHUNK_LEN
                elif self._is_client:
                    self._push_data()
                    self._state = self.__METHOD
                else:
                    self._push_data()
                    self._state = self.__RESP
            else:
                sp = line.split(b': ')

                self._header[sp[0].lower()] = b': '.join(sp[1:])

            return True
        else:
            return False

    def _skip_body(self):
        while len(self._data) > 0:
            num = sum([len(x) for x in self._data[0]])
            if num <= self._remain:
                self._data.pop(0)
                self._remain -= num

                if self._remain == 0:
                    if self._is_client:
                        self._push_data()
                        self._state = self.__METHOD
                    else:
                        self._push_data()
                        self._state = self.__RESP
            else:
                while True:
                    num = len(self._data[0][0])
                    if num <= self._remain:
                        self._data[0].pop(0)
                        self._remain -= num
                    else:
                        self._data[0][0] = self._data[0][0][self._remain:]
                        self._remain  = 0
                    if self._remain == 0:
                        if self._is_client:
                            self._push_data()
                            self._state = self.__METHOD
                        else:
                            self._push_data()
                            self._state = self.__RESP

                        return

    def _read_line(self):
        line = b''
        for i, v in enumerate(self._data):
            for j, buf in enumerate(v):
                idx = buf.find(b'\n')
                if idx >= 0:
                    line += buf[:idx].rstrip()

                    self._data[i] = v[j:]

                    suffix = buf[idx + 1:]

                    if len(suffix) > 0:
                        self._data[i][0] = suffix
                    else:
                        self._data[i].pop(0)

                    if len(self._data[i]) > 0:
                        self._data = self._data[i:]
                    else:
                        self._data = self._data[i + 1:]

                    return (True, line)
                else:
                    line += buf

        return (False, None)

class sftap_http:
    def __init__(self):
        self._content = []

        self._conn = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self._conn.connect(uxpath)

        self._header = {}

        self.__HEADER = 0
        self.__DATA   = 1
        self._state = self.__HEADER

        self._http = {}

    def run(self):
        while True:
            buf = b'' + self._conn.recv(65536)
            if len(buf) == 0:
                print('remote socket was closed')
                return

            self._content.append(buf)
            self._parse()

    def _parse(self):
        while True:
            if self._state == self.__HEADER:
                (result, line) = self._read_line()
                if result == False:
                    break

                self._header = self._parse_header(line)

                if self._header[b'event'] == b'DATA':
                    self._state = self.__DATA
                elif self._header[b'event'] == b'CREATED':
                    id = self._get_id()
                    self._http[id] = (http_parser(is_client = True),
                                      http_parser(is_client = False))
                elif self._header[b'event'] == b'DESTROYED':
                    try:
                        id = self._get_id()
                        del self._http[id]
                    except KeyError:
                        pass
            elif self._state == self.__DATA:
                num = int(self._header[b'len'])

                (result, buf) = self._read_bytes(num)
                if result == False:
                    break

                id = self._get_id()

                if id in self._http:
                    if self._header[b'match'] == b'up':
                        self._http[id][0].in_data(buf, self._header)
                    elif self._header[b'match'] == b'down':
                        self._http[id][1].in_data(buf, self._header)
                else:
                    pass

                self._state = self.__HEADER
            else:
                print("ERROR: unkown state")
                exit(1)

    def _read_line(self):
        line = b''
        for i, buf in enumerate(self._content):
            idx = buf.find(b'\n')
            if idx >= 0:
                line += buf[:idx]

                self._content = self._content[i:]

                suffix = buf[idx + 1:]

                if len(suffix) > 0:
                    self._content[0] = suffix
                else:
                    self._content.pop(0)

                return (True, line)
            else:
                line += buf

        return (False, b'')

    def _read_bytes(self, num):
        n = 0
        for buf in self._content:
            n += len(buf)

        if n < num:
            return (False, None)

        data = []
        while True:
            buf = self._content.pop(0)
            if len(buf) <= num:
                data.append(buf)
                num -= len(buf)
            else:
                d = buf[:num]
                data.append(d)
                self._content.insert(0, buf[num:])
                num -= len(d)

            if num == 0:
                return (True, data)

        return (False, None)

    def _parse_header(self, line):
        d = {}
        for x in line.split(b','):
            m = x.split(b'=')
            d[m[0]] = m[1]

        return d

    def _get_id(self):
        return (self._header[b'ip1'],
                self._header[b'ip2'],
                self._header[b'port1'],
                self._header[b'port2'],
                self._header[b'hop'])

def main():
    parser = sftap_http()
    parser.run()

if __name__ == '__main__':
    main()
