#!/usr/bin/env python
# -*- coding: utf-8 -*-

import optparse
import atexit
import importlib.util
import subprocess
import signal
import time

if __name__ == '__main__':
    parser = optparse.OptionParser()
    parser.add_option('-s', dest='server', type='str', default=None,
                      help='server executable path')
    parser.add_option('-c', dest='server_config', type='str', default=None,
                      help='server config path')
    parser.add_option('-l', dest='lemma_server', type='str', default=None,
                      help='lemma server executable path')
    parser.add_option('-d', dest='database', type='str', default=None,
                      help='name of server database without extension')
    parser.add_option('-D', dest='lemma_database', default=False, action="store_true",
                      help='ask lemma_server to dump lemmas into <server_database>.lemma.db')

    options, args = parser.parse_args()

    if not options.server:
        parser.error("-s server_path required")

    if not options.server_config:
        parser.error("-c server_config required")

    args = ['/usr/bin/env', 'python3', options.server, '-c', options.server_config]
    if options.database:
        args += ['-d', options.database + '.db']

    server = subprocess.Popen(args)


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
        time.sleep(5)
    except KeyboardInterrupt:
        pass

    if options.lemma_server:
        try:
            spec = importlib.util.spec_from_file_location("config", options.server_config)
            config = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(config)
        except:
            print('cannot run lemma server')
        else:
            args = [options.lemma_server, '-s', '127.0.0.1:' + str(config.port)]
            if options.lemma_database and options.database:
                args += ['-d', options.database + '.lemma.db']
            lemma_server = subprocess.Popen(args)

    try:
        server.wait()
    except KeyboardInterrupt:
        pass
