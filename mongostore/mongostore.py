#!/usr/bin/env python
# -*- coding:utf-8 -*-

import argparse
import json
import pymongo
from pymongo import MongoClient
from datetime import datetime

class MongoStore:
    def __init__(self, args):
        self.con = MongoClient(args.server, args.port)
        self.col = self.con[args.db][args.collection]

        self.verbose = args.verbose

    def run(self):
        while True:
            try:
                line = input()
                data = json.loads(line)
                data['timestamp'] = datetime.now()
                self.col.insert(data)

                if self.verbose:
                    print(data)
            except ValueError:
                pass

def parse_args():
    parser = argparse.ArgumentParser(description='store JSON to Elasticsearch')

    parser.add_argument('-s', dest='server', default='localhost',
                        help='server address to MongoDB (default = localhost)')
    parser.add_argument('-p', dest='port', default=27017, type=int,
                        help='port numbers to MongoDB (default = 27017)')
    parser.add_argument('-d', dest='db', required=True,
                        help='database of MongoDB')
    parser.add_argument('-c', dest='collection', required=True,
                        help='collection for MongoDB')
    parser.add_argument('-v', dest='verbose', default=False, action='store_true',
                        help='enable verbose mode')


    return parser.parse_args()

def main():
    args = parse_args()

    mongo = MongoStore(args)
    mongo.run()

if __name__ == '__main__':
    main()
