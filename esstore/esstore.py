#!/usr/bin/env python
# -*- coding:utf-8 -*-

import argparse
import json
from datetime import datetime
from elasticsearch import Elasticsearch

class EStore:
    def __init__(self, args):
        self.es = Elasticsearch(args.server)

        self.index   = args.index
        self.type    = args.type
        self.verbose = args.verbose

    def run(self):
        while True:
            line = input()
            data = json.loads(line)
            data['timestamp'] = datetime.utcnow()
            self.es.index(index=self.index, doc_type=self.type, body=data)

            if self.verbose:
                print(data)

def parse_args():
    parser = argparse.ArgumentParser(description='store JSON to Elasticsearch')

    parser.add_argument('-s', dest='server', default=['localhost:9200'], nargs='+',
                        help='server address to Elasticsearch (default = localhost:9200)')
    parser.add_argument('-i', dest='index', required=True,
                        help='index for Elasticsearch')
    parser.add_argument('-t', dest='type', required=True,
                        help='type for Elasticsearch')
    parser.add_argument('-v', dest='verbose', default=False, action='store_true',
                        help='enable verbose mode')

    return parser.parse_args()

def main():
    args = parse_args()

    es = EStore(args)
    es.run()

if __name__ == '__main__':
    main()
