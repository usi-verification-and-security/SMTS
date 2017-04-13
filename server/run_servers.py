#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import optparse
import atexit
import importlib.util
import subprocess
import signal
import pathlib
import time
import socket
import config
import sys

if __name__ == '__main__':

    path = str(pathlib.Path(__file__).parent.resolve()) + '/'

    custom_config = []


    def callback_config(option, opt, value, parser):
        global custom_config
        custom_config += ['-c', value]
        config.extend(value)


    parser = optparse.OptionParser()
    parser.add_option('-c', '--config', dest='config_path', type='str',
                      action="callback", callback=callback_config,
                      help='config file path')
    parser.add_option('-l', dest='lemma_server', type='str', default=None,
                      help='lemma server executable path')
    parser.add_option('-d', dest='database', type='str', default=None,
                      help='name of server database without extension')
    parser.add_option('-D', dest='lemma_database', default=False, action="store_true",
                      help='ask lemma_server to dump lemmas')
    parser.add_option('-a', dest='lemma_send_again', default=False, action="store_true",
                      help='lemma_server -a option')

    options, args = parser.parse_args()

    ip = '127.0.0.1'
    try:
        ip = socket.gethostbyname(socket.gethostname())
    except:
        pass

    args = ['/usr/bin/env', 'python3', str(pathlib.Path(__file__).parent / 'server.py')] + custom_config

    if options.database:
        args += ['-d', options.database + '.db']

    print(' '.join(args))
    try:
        server = subprocess.Popen(args)
    except BaseException as ex:
        print(ex)
        sys.exit(-1)


    def kill_server(force=False):
        if force:
            server.kill()
        else:
            server.send_signal(signal.SIGINT)
        try:
            server.wait()
        except:
            kill_server(True)


    atexit.register(kill_server)

    try:
        time.sleep(3)
    except KeyboardInterrupt:
        pass

    if options.lemma_server:
        args = [options.lemma_server, '-s', ip + ':' + str(config.port)]
        if options.lemma_send_again:
            args += ['-a']
        if options.lemma_database and options.database:
            args += ['-d', options.database + '.lemma.db']
        print(' '.join(args))
        try:
            lemma_server = subprocess.Popen(args)
        except BaseException as ex:
            print(ex)

    try:
        server.wait()
    except KeyboardInterrupt:
        pass
