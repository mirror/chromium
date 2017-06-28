import json
from subprocess import call

tests_so_far = []

for i in range(0, 6):
    # call(['./run-webkit-tests', '-t', 'Release', 'external/wpt/css/CSS2', '--shard-index', str(i), '--total-shards', '6', '--seed', '4'])
    call(['./run-webkit-tests', '-t', 'Release', '--shard-index', str(i), '--total-shards', '6', '--seed', '4'])

    with open('/usr/local/google/home/jeffcarp/tests-to-run.json') as f:
        tests_to_run = json.load(f)
        print('shard', i, 'tests', len(tests_to_run), 'tests_so_far',
            len(tests_so_far), '----------------------------------')

    for test in tests_to_run:
        if test in tests_so_far:
            'test already exists! %s' % test

        tests_so_far.append(test)
