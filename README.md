# Binjecter

A static binary injection tool

## Usage

Find your executable to inject, your payload and wether you want the program to change the entrypoint. Also get a random short name for the section injected. Once that's done, modify the find_base_addr.bash to use your arguments then launch it using `./find_base_addr.bash` in your terminal.
Then you should get a list of address after some time, take any then use the program like so :

```
/Binjecter -f [your executable] -p [your payload] -e [1=change entrypoint, 1=dont and exploit got] -s [section name] -a [address found]
```

## Note

You can use `make stuff` to use the sample payloads and elf program and then use :
```
./Binjecter -f ./date -p ./payload_got -e 0 -s coucou -a 0x600b70
or
./Binjecter -f ./date -p ./payload_inject -e 1 -s coucou -a 0x600b70
```