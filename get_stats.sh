#!/bin/bash

OLDIFS="$IFS";
IFS=$' '

read_bytes=0
write_bytes=0
files_sent=0
files_received=0
max_id=0
min_id=10000000000000
clients=0
list_of_clients=""
clients_leaved=0

#read line by line the stdin
while read line
do
    chunks=$line
    for x in $chunks #take each word from the line 
    do
	if [[ "$next_read" -eq 1 ]] #if previous word was Read
        then
            read_bytes=$(($read_bytes+$x))
            next_read=0
        fi

        if [ "$x"  = "Read" ] 
        then
            next_read=1  #if this word is Read mark it for the next iteration
        fi
        ###############################################
        if [[ "$next_write" -eq 1 ]] #if previous word was Wrote
        then
            write_bytes=$(($write_bytes+$x))
            next_write=0
        fi

        if [ "$x"  = "Wrote" ] 
        then
            next_write=1 #if this word is Wrote mark it for the next iteration
        fi
        ###############################################
        if [[ "$next_id" -eq 1  ]] #if previous word was id
        then
            if [ $x -gt $max_id ]
            then
                max_id=$x #find max id 
            fi

            if [ $x -lt $min_id ]
            then
                min_id=$x #find min id
            fi
            clients=$((clients+1)) #count the clients
            list_of_clients="$list_of_clients | $x" #put clients in a list 
            next_id=0
        fi

        if [ "$x"  = "id" ] 
        then
            next_id=1 #if this word is id mark it for the next iteration
        fi
        ###############################################
        if [ "$x"  = "sent" ]
        then
            files_sent=$(($files_sent+1)) #if this word is sent count it 
        fi

        if [ "$x" = "received" ]
        then
            files_received=$(($files_received+1)) #if this word is received count it 
        fi
        ###############################################
        if [ "$x" = "leaved" ]
        then
            clients_leaved=$(($clients_leaved+1)) #if this word is leaved count it
        fi
    done
done

#echo the informations 

echo "Read bytes:       " $read_bytes
echo "Write bytes:      " $write_bytes
echo "Files sent:       " $files_sent
echo "Files received:   " $files_received
echo "Max id:           " $max_id
echo "Min id:           " $min_id
echo "Clients leaved:   " $clients_leaved
echo "All clients:      " $clients
echo -n "List of id(s):    "
for x in $list_of_clients
do
    echo -n $x
done
echo "|"

IFS="$OLDIFS"
