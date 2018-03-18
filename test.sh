#!/bin/bash

set +e

DATA="FOO"
RESULT=0

docker run \
    --name tcp-echo-test \
    --rm \
    tcp-echo-test \
    /bin/bash -c "valgrind --error-exitcode=1 --leak-check=full --show-leak-kinds=all --track-origins=yes --trace-children=yes ./tcp-echo-master" &

for i in {1..30}; do
    RETURNED_DATA=$(docker exec tcp-echo-test /bin/bash -c "echo ${DATA}| nc -w 1 localhost 8090" 2> /dev/null)

    EXIT_CODE=$?

    if [ ${EXIT_CODE} -eq 0 ]; then
        echo "Received (${RETURNED_DATA})"
        break;
    fi

    sleep 1
done

docker kill -s SIGTERM tcp-echo-test > /dev/null 2>&1

if [ ${EXIT_CODE} -ne 0 ]; then
    echo "Could not connect"
    RESULT=1
fi

if [ "${RETURNED_DATA}" != "${DATA}" ] ; then
    echo "Data received (${RETURNED_DATA}) did not match (${DATA})"
    RESULT=1
fi

wait $!

exit $(($? + ${RESULT}))
