#!/bin/bash

make shit >/dev/null
base_addr=0x600000

for ((i = 0 ; i < 0x400000 ; i++)); do
    cp ./date ./date_tmp
    ./Binjecter -f ./date_tmp -p ./payload_inject -a $base_addr -s coucou -e 1 >/dev/null

    if [[ -n `./date_tmp` ]]; then
        echo "It works with addr $base_addr"
    fi

    printf -v base_addr '%#X' "$((base_addr + 0x1))"
done


rm ./date_tmp


#./Binjecter -f ./date -p ./payload -a 0X41C254 -s coucou -e 1