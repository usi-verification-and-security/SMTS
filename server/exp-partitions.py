import net
import sys
import pathlib
import zipfile

s = net.Socket()
s.listen(("127.0.0.1", 5000))
c = s.accept()
c.read()
for path in sys.argv[1:]:
    remove = False
    if path.endswith(".zip"):
        z = zipfile.ZipFile(path)
        path = z.namelist()[0]
        z.extract(path)
        remove = True
    with open(path, 'r') as file:
        print(path)
        content = file.read()
        c.write({"command": "solve", "name": path, "node": ""}, content)
        c.write({"command": "partition", "partitions": "2", "name": path, "node": ""}, "")
        while True:
            header, payload = c.read()
            print(header)
            if "name" not in header or header["name"] != path:
                continue
            if "partitions" in header:
                lines = content.split('\n');
                for index, partition in enumerate(payload.decode().split('\n')):
                    partition_smt = '\n'.join(lines[:lines.index('(check-sat)')]) + \
                                    '\n(push 1)\n(assert ' + partition + ')\n(check-sat)\n(exit)\n'
                    # print(partition_smt)
                    out = pathlib.Path(
                        'experiments/' + pathlib.Path(path).name + '.' + str(index + 1) + '.part.smt2')
                    try:
                        out.parent.mkdir(parents=True)
                    except:
                        pass
                    out.open('w').write(partition_smt)
                break
            elif "status" in header:
                break

    if remove:
        pathlib.Path(path).unlink()