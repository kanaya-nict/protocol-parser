import socket

uxpath = '/tmp/sf-tap/tcp/http'

class http_parser:
    def __init__(self):
        self.__HEADER  = 1
        self.__BODY    = 2
        self.__CHUNK   = 3
        self.__TRAILER = 4

        self._state = self.__HEADER

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
            buf = self._conn.recv(65536)
            if len(buf) == 0:
                print 'remote socket was closed'
                return

            self._content.append(buf)
            self._parse()

    def _parse(self):
        while True:
            if self._state == self.__HEADER:
                (result, line) = self._content_read_line()
                if result == False:
                    break

                self._header = self._parse_header(line)

                print self._header

                if self._header['event'] == 'DATA':
                    self._state = self.__DATA
                elif self._header['event'] == 'CREATED':
                    self._http[self._get_id()] = http_parser()
                elif self._header['event'] == 'DESTROYED':
                    try:
                        del self._http[self._get_id()]
                    except KeyError:
                        pass
            elif self._state == self.__DATA:
                num = int(self._header['len'])

                (result, buf) = self._content_read_bytes(num)
                if result == False:
                    break

                self._state = self.__HEADER

            else:
                print "ERROR: unkown state"
                exit(1)

    def _content_read_line(self):
        i = 0
        line = ''
        for buf in self._content:
            try:
                idx = buf.index('\n')
                line += buf[0:idx]

                self._content    = self._content[i:]

                suffix = buf[idx + 1:]

                if len(suffix) > 0:
                    self._content[0] = suffix
                else:
                    self._content.pop(0)

                return (True, line)
            except ValueError:
                line += data
                i += 1

        return (False, '')

    def _content_read_bytes(self, num):
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
                d = buf[0:num]
                data.append(d)
                self._content.insert(0, buf[num:])
                num -= len(d)

            if num == 0:
                return (True, data)

        return (False, None)

    def _parse_header(self, line):
        d = {}
        for x in line.split(','):
            m = x.split('=')
            d[m[0]] = m[1]

        return d

    def _get_id(self):
        (self._header['ip1'],
         self._header['ip2'],
         self._header['port1'],
         self._header['port2'],
         self._header['hop'])

def main():
    parser = sftap_http()
    parser.run()

if __name__ == '__main__':
    main()
